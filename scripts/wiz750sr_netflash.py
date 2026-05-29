#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
OTA-flash the App of a WIZ750SR module over the network via SEGCP.

Flow (see s2e/app/src/Configuration/segcp.c and PlatformHandler/deviceHandler.c):
  1. [Optional] When -m is given without -i, discover the device IP via a
     targeted UDP SEGCP broadcast: the packet is addressed to the device MAC
     so only that module replies, and its UDP source IP becomes the target.
  2. Open TCP to <device-ip>:50001 (SEGCP) and query VR / MC / MN with the
     broadcast MAC (READ privilege). Records the BEFORE version.
  3. Send the SEGCP AB command (AppBoot mode) to the device (WRITE privilege),
     then poll the SEGCP port until the device finishes rebooting into its
     AppBoot firmware (chip reset + clock init + PHY auto-negotiation can take
     well over 5 s, so a fixed wait + single connect attempt is unreliable).
  4. Open a second TCP exchange targeting the device's REAL MAC
     (WRITE privilege) with command "FW<size>". The device replies
     "FW<ip>:50002\\r\\n" and immediately switches to a TCP-listen
     state on port 50002, erasing the App backup sector in the
     background.
  5. Connect TCP to <ip>:50002 (with retries — erase takes seconds)
     and stream the firmware bytes.
  6. Wait for the device to swap backup -> main and reboot, then
     re-query VR. Records the AFTER version.

Two modes:
  - default (interactive): pretty-printed progress with versions
    before / after.
  - --script: machine-readable TSV on stdout
    "mac<TAB>ip<TAB>before_version<TAB>after_version<TAB>status"
    Diagnostic messages go to stderr. Exit 0 on success, 1 on failure.
"""

from __future__ import annotations

import argparse
import re
import socket
import sys
import time
from pathlib import Path

SEGCP_PORT = 50001
FWUP_PORT = 50002
BROADCAST_MAC = b"\xff\xff\xff\xff\xff\xff"
DEFAULT_FWUP_SIZE_LIMIT = 100 * 1024  # DEVICE_FWUP_SIZE (100 KB without __USE_APPBACKUP_AREA__)

APPBOOT_WAIT = 3.0       # initial settle delay after AB before polling SEGCP
APPBOOT_POLL = 45.0      # max time to wait for AppBoot SEGCP to come back up

MAC_RE = re.compile(r"^[0-9A-Fa-f]{2}([:\-.])(?:[0-9A-Fa-f]{2}\1){4}[0-9A-Fa-f]{2}$")
FW_REPLY_RE = re.compile(r"^(\d+\.\d+\.\d+\.\d+):(\d+)$")


def parse_mac(s: str) -> bytes:
    if not MAC_RE.match(s):
        raise argparse.ArgumentTypeError(
            f"invalid MAC {s!r}: expected 6 hex octets with ':' '-' or '.' separators"
        )
    return bytes.fromhex(re.sub(r"[:\-.]", "", s))


def parse_segcp_reply(data: bytes) -> dict[str, str] | None:
    """Decode a SEGCP reply. Header is binary 'MA<6 bytes>\\r\\n', then
    ASCII tokens 'PW...', '<2cmd><value>...' joined by \\r\\n."""
    if len(data) < 10 or not data.startswith(b"MA"):
        return None
    fields: dict[str, str] = {
        "mac": ":".join(f"{b:02X}" for b in data[2:8]),
    }
    for tok in data[10:].decode("ascii", "replace").split("\r\n"):
        if len(tok) >= 2 and tok[:2].isalpha():
            fields[tok[:2]] = tok[2:]
    return fields


def discover_ip_by_mac(
    mac_bytes: bytes,
    *,
    broadcast: str = "255.255.255.255",
    timeout: float = 3.0,
    port: int = SEGCP_PORT,
) -> str | None:
    """Send a targeted UDP SEGCP discovery packet addressed to mac_bytes and
    return the source IP of the first matching reply, or None on timeout."""
    packet = (
        b"MA" + mac_bytes + b"\r\n"
        + b"PW\r\n"
        + b"VR\r\n"
    )
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", 0))
    sock.settimeout(0.2)
    try:
        sock.sendto(packet, (broadcast, port))
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                data, addr = sock.recvfrom(4096)
            except socket.timeout:
                continue
            info = parse_segcp_reply(data)
            if info is None:
                continue
            reply_mac = bytes.fromhex(info.get("mac", "").replace(":", ""))
            if reply_mac == mac_bytes:
                return addr[0]
    finally:
        sock.close()
    return None


def segcp_exchange(
    ip: str, mac_target: bytes, password: str, commands: list[str],
    *, connect_timeout: float = 3.0, quiet_timeout: float = 0.5,
) -> dict[str, str] | None:
    """One TCP SEGCP exchange. Returns parsed reply or None if no SEGCP frame."""
    packet = (
        b"MA" + mac_target + b"\r\n"
        + b"PW" + password.encode("ascii") + b"\r\n"
        + b"".join(c.encode("ascii") + b"\r\n" for c in commands)
    )
    chunks: list[bytes] = []
    with socket.create_connection((ip, SEGCP_PORT), timeout=connect_timeout) as s:
        s.sendall(packet)
        s.settimeout(quiet_timeout)
        try:
            while True:
                data = s.recv(4096)
                if not data:
                    break
                chunks.append(data)
        except socket.timeout:
            pass
    return parse_segcp_reply(b"".join(chunks))


def segcp_exchange_retry(
    ip: str, mac_target: bytes, password: str, commands: list[str],
    *, total_wait: float = 30.0, retry_delay: float = 1.0, log=None, **kw,
) -> dict[str, str] | None:
    """Like segcp_exchange but keeps retrying while the TCP connect fails.

    After an AB (AppBoot) reboot the device is unreachable for several seconds
    (chip reset + 8 MHz clock init + Ethernet PHY auto-negotiation), so a single
    3 s connect attempt times out before the AppBoot SEGCP socket is listening.
    We poll until the device answers or total_wait elapses, then raise the last
    connection error so the caller can report a real timeout.
    """
    deadline = time.monotonic() + total_wait
    last_exc: Exception | None = None
    while True:
        try:
            return segcp_exchange(ip, mac_target, password, commands, **kw)
        except (OSError, socket.timeout) as exc:
            last_exc = exc
            if time.monotonic() >= deadline:
                raise
            if log is not None:
                print("[*] AppBoot not reachable yet, retrying ...", file=log)
            time.sleep(retry_delay)


def send_appboot_cmd(ip: str, mac_bytes: bytes, password: str, *, log) -> None:
    """Send the AB (AppBoot) SEGCP command then wait for the device to reboot."""
    print(f"[*] Sending AB (AppBoot) command to {ip} ...", file=log)
    try:
        segcp_exchange(ip, mac_bytes, password, ["AB"])
    except (OSError, socket.timeout):
        # Device reboots and drops the TCP connection — this is expected.
        pass
    print(f"[*] Waiting {APPBOOT_WAIT:.0f}s for AppBoot reboot ...", file=log)
    time.sleep(APPBOOT_WAIT)


CHUNK_SIZE = 1024      # bytes per chunk — matches WIZnet s2e-tool reference
ACK_TIMEOUT = 5.0     # seconds to wait for the 2-byte ACK per chunk (DEVICE_FWUP_TIMEOUT)


def upload_firmware(
    ip: str, port: int, payload: bytes,
    *, connect_retries: int = 15, retry_delay: float = 1.0, connect_timeout: float = 10.0,
) -> None:
    """Connect to <ip>:<port> and stream the payload in CHUNK_SIZE-byte blocks.

    After each chunk the device sends a 2-byte big-endian ACK whose value is
    the number of bytes it actually received and wrote to flash (see
    boot/src/PlatformHandler/deviceHandler.c get_firmware_from_network()).
    We wait for that ACK before sending the next chunk so we never overflow
    the device's 2 KB socket RX buffer during the post-connect flash erase.
    """
    last_exc: Exception | None = None
    for _ in range(connect_retries):
        try:
            with socket.create_connection((ip, port), timeout=connect_timeout) as s:
                offset = 0
                while offset < len(payload):
                    chunk = payload[offset : offset + CHUNK_SIZE]
                    s.sendall(chunk)
                    # Read 2-byte ACK (big-endian length acknowledged by device).
                    s.settimeout(ACK_TIMEOUT)
                    ack = b""
                    while len(ack) < 2:
                        part = s.recv(2 - len(ack))
                        if not part:
                            raise IOError(
                                f"connection closed by device after {offset} bytes "
                                f"(expected ACK for chunk at offset {offset})"
                            )
                        ack += part
                    acked = (ack[0] << 8) | ack[1]
                    if acked == 0:
                        raise IOError(
                            f"device sent zero-length ACK at offset {offset}"
                        )
                    offset += acked
                return
        except (OSError, socket.timeout) as exc:
            last_exc = exc
            time.sleep(retry_delay)
    raise IOError(
        f"upload to {ip}:{port} failed after {connect_retries} attempts: {last_exc}"
    )


def wait_for_reboot(
    ip: str, password: str, *, max_wait: float = 30.0, poll: float = 1.0,
) -> dict[str, str] | None:
    """Poll the SEGCP port until the device answers a VR query."""
    deadline = time.monotonic() + max_wait
    while time.monotonic() < deadline:
        try:
            info = segcp_exchange(ip, BROADCAST_MAC, password, ["VR", "MC", "MN"])
            if info:
                return info
        except (OSError, socket.timeout):
            pass
        time.sleep(poll)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="OTA-flash the App of a WIZ750SR module via SEGCP / TCP.",
    )
    parser.add_argument("firmware", type=Path, help="Path to App .bin to flash (≤ 100 KB).")
    parser.add_argument("-i", "--ip", default=None, help="Device IP address.")
    parser.add_argument(
        "-m", "--mac", type=parse_mac, default=None,
        help="Device MAC. If -i is omitted, used to discover the device IP via UDP broadcast. "
             "Also required for SEGCP write privilege; auto-discovered from the BEFORE query if omitted.",
    )
    parser.add_argument(
        "-B", "--broadcast", default="255.255.255.255",
        help="Broadcast address for MAC-based IP discovery (default: 255.255.255.255). "
             "Use 192.168.x.255 to scope to one subnet.",
    )
    parser.add_argument(
        "-P", "--password", default="",
        help="Device search password (default: empty).",
    )
    parser.add_argument(
        "--reboot-wait", type=float, default=20.0,
        help="Seconds to wait for the device to come back after reboot (default: 20).",
    )
    parser.add_argument(
        "-s", "--script", action="store_true",
        help="Machine-readable TSV on stdout (mac, ip, before_ver, after_ver, status); "
             "all diagnostics on stderr. Exit 0 on success.",
    )
    args = parser.parse_args()

    log = sys.stderr if args.script else sys.stdout

    if args.ip is None and args.mac is None:
        parser.error("at least one of -i/--ip or -m/--mac is required")

    if not args.firmware.is_file():
        print(f"[!] firmware not found: {args.firmware}", file=sys.stderr)
        return 2
    payload = args.firmware.read_bytes()
    if not payload:
        print("[!] firmware file is empty", file=sys.stderr)
        return 2
    if len(payload) > DEFAULT_FWUP_SIZE_LIMIT:
        print(
            f"[!] firmware size {len(payload)} > {DEFAULT_FWUP_SIZE_LIMIT} bytes; "
            "device will reject the FW command.", file=sys.stderr,
        )
        return 2

    print(f"[*] Firmware  : {args.firmware} ({len(payload)} bytes)", file=log)

    # --- Step 0: Discover IP by MAC if -i was not provided ---
    if args.ip is None:
        mac_str = ":".join(f"{b:02X}" for b in args.mac)
        print(f"[*] Discovering IP for MAC {mac_str} (broadcast {args.broadcast}) ...", file=log)
        args.ip = discover_ip_by_mac(args.mac, broadcast=args.broadcast)
        if args.ip is None:
            print(f"[!] no reply from MAC {mac_str} on {args.broadcast}:{SEGCP_PORT}", file=sys.stderr)
            if args.script:
                print(f"{mac_str}\t\t\t\tUNREACHABLE")
            return 1
        print(f"[*] Found     : {mac_str} at {args.ip}", file=log)

    print(f"[*] Target    : {args.ip}", file=log)

    # --- Step 1: BEFORE query (broadcast MAC = READ privilege) ---
    print(f"[*] Querying device for current version ...", file=log)
    try:
        before = segcp_exchange(args.ip, BROADCAST_MAC, args.password, ["VR", "MC", "MN", "ST"])
    except (OSError, socket.timeout) as exc:
        print(f"[!] cannot reach device at {args.ip}:{SEGCP_PORT}: {exc}", file=sys.stderr)
        if args.script:
            print(f"\t{args.ip}\t\t\tUNREACHABLE")
        return 1
    if before is None:
        print(f"[!] no SEGCP reply from {args.ip}", file=sys.stderr)
        if args.script:
            print(f"\t{args.ip}\t\t\tNO_REPLY")
        return 1

    before_mac = before.get("mac", "")
    before_ver = before.get("VR", "")
    before_state = before.get("ST", "")
    print(
        f"[*] Current   : version={before_ver or '?'} mac={before_mac or '?'} "
        f"name={before.get('MN', '?')} state={before_state or '?'}", file=log,
    )

    # --- Resolve device MAC for write privilege ---
    if args.mac is None:
        try:
            mac_bytes = bytes.fromhex(before_mac.replace(":", ""))
            assert len(mac_bytes) == 6
        except (ValueError, AssertionError):
            print(f"[!] cannot derive device MAC from reply ({before_mac!r})", file=sys.stderr)
            return 1
        args.mac = mac_bytes

    # --- Step 1b: Switch to AppBoot mode if not already there ---
    if before_state == "BOOT":
        print(f"[*] Already in AppBoot mode, skipping AB command.", file=log)
    else:
        send_appboot_cmd(args.ip, args.mac, args.password, log=log)

    # --- Step 2: Trigger FW SET ---
    print(f"[*] Sending FW{len(payload)} (initiate OTA) ...", file=log)
    try:
        fw_reply = segcp_exchange_retry(
            args.ip, args.mac, args.password, [f"FW{len(payload)}"],
            total_wait=APPBOOT_POLL, log=log,
        )
    except (OSError, socket.timeout) as exc:
        print(f"[!] FW command failed: {exc}", file=sys.stderr)
        if args.script:
            print(f"{before_mac}\t{args.ip}\t{before_ver}\t\tFW_CMD_FAILED")
        return 1
    if fw_reply is None or "FW" not in fw_reply:
        print(f"[!] device did not return 'FW<ip>:<port>' (got {fw_reply!r})", file=sys.stderr)
        if args.script:
            print(f"{before_mac}\t{args.ip}\t{before_ver}\t\tFW_CMD_REJECTED")
        return 1
    m = FW_REPLY_RE.match(fw_reply["FW"])
    if not m:
        print(f"[!] cannot parse FW reply {fw_reply['FW']!r}", file=sys.stderr)
        return 1
    fwup_ip, fwup_port = m.group(1), int(m.group(2))
    print(f"[*] Uploading to {fwup_ip}:{fwup_port} ...", file=log)

    # --- Step 3: Stream the payload (with retries for erase delay) ---
    t0 = time.monotonic()
    try:
        upload_firmware(fwup_ip, fwup_port, payload)
    except IOError as exc:
        print(f"[!] {exc}", file=sys.stderr)
        if args.script:
            print(f"{before_mac}\t{args.ip}\t{before_ver}\t\tUPLOAD_FAILED")
        return 1
    print(f"[+] Upload OK ({len(payload)} bytes in {time.monotonic() - t0:.1f}s)", file=log)

    # --- Step 4: Wait for reboot, AFTER query ---
    print(f"[*] Waiting up to {args.reboot_wait:.0f}s for reboot ...", file=log)
    after = wait_for_reboot(args.ip, args.password, max_wait=args.reboot_wait)
    if after is None:
        print(f"[!] device did not respond within {args.reboot_wait:.0f}s after upload", file=sys.stderr)
        if args.script:
            print(f"{before_mac}\t{args.ip}\t{before_ver}\t\tUNREACHABLE_AFTER")
        return 1
    after_ver = after.get("VR", "")
    print(f"[*] After     : version={after_ver or '?'}", file=log)

    status = "OK" if after_ver else "OK_NO_VERSION"
    if args.script:
        print(f"{after.get('mac', before_mac)}\t{args.ip}\t{before_ver}\t{after_ver}\t{status}")
    else:
        print(f"[+] OTA complete: {before_ver or '?'} -> {after_ver or '?'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
