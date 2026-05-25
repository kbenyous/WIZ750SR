#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
Configure one or several WIZ750SR modules to obtain their IP via DHCP.

The script sends a SEGCP unicast UDP command to each target device:
  MA<device-mac>  → grants WRITE privilege (device's real MAC, not broadcast)
  PW              → search password (empty by default)
  IM1             → IP address method = DHCP  (0=static, 1=DHCP)
  SV              → save settings to flash
  RT              → reboot to apply

After the reboot delay, a new broadcast discovery is performed and the
updated device list (MAC, IP, version) is printed.

Target selection (mutually exclusive, one required):
  --all                  configure every device that responds to discovery
  --mac AA:BB:CC:DD:EE:FF [...]  configure only the listed device(s)

Protocol notes (s2e/app/src/Configuration/segcp.c):
  - Devices listen on UDP/50001.
  - Targeting the device's real MAC grants SEGCP_PRIVILEGE_SET |
    SEGCP_PRIVILEGE_WRITE, which allows IM, SV and RT.
  - The device replies with its status; after RT it reboots and the reply
    may arrive before the reboot or not at all — that is normal.

Networking caveats:
  - WSL2's default NAT does NOT forward UDP broadcasts. Use mirrored
    networking mode or run from Windows.
  - On hosts with multiple interfaces, use -B <subnet>.255 to scope
    the discovery broadcast to the right LAN.
"""

from __future__ import annotations

import argparse
import platform
import re
import socket
import sys
import time
from pathlib import Path

SEGCP_PORT = 50001
BROADCAST_MAC = b"\xff\xff\xff\xff\xff\xff"

MAC_RE = re.compile(
    r"^[0-9A-Fa-f]{2}([:\-.])(?:[0-9A-Fa-f]{2}\1){4}[0-9A-Fa-f]{2}$"
)


# ---------------------------------------------------------------------------
# MAC helpers
# ---------------------------------------------------------------------------

def normalize_mac(s: str) -> str:
    """Validate and return canonical 'AA:BB:CC:DD:EE:FF' (uppercase)."""
    if not MAC_RE.match(s):
        raise argparse.ArgumentTypeError(
            f"invalid MAC {s!r}: expected 6 hex octets with ':' '-' or '.' separators"
        )
    octets = re.split(r"[:\-.]", s)
    return ":".join(o.upper() for o in octets)


def mac_str_to_bytes(mac: str) -> bytes:
    return bytes.fromhex(mac.replace(":", ""))


# ---------------------------------------------------------------------------
# SEGCP packet builder / parser
# ---------------------------------------------------------------------------

def build_segcp_packet(mac_target: bytes, password: str, commands: list[str]) -> bytes:
    return (
        b"MA" + mac_target + b"\r\n"
        + b"PW" + password.encode("ascii") + b"\r\n"
        + b"".join(c.encode("ascii") + b"\r\n" for c in commands)
    )


def parse_segcp_reply(data: bytes) -> dict[str, str] | None:
    """Decode a SEGCP UDP reply. Header: 'MA<6 bytes>\\r\\n', then ASCII tokens."""
    if len(data) < 10 or not data.startswith(b"MA"):
        return None
    mac_bin = data[2:8]
    if mac_bin == BROADCAST_MAC:
        return None
    fields: dict[str, str] = {
        "mac": ":".join(f"{b:02X}" for b in mac_bin),
    }
    for tok in data[10:].decode("ascii", errors="replace").split("\r\n"):
        if len(tok) >= 2 and tok[:2].isalpha():
            fields[tok[:2]] = tok[2:]
    return fields


# ---------------------------------------------------------------------------
# Discovery
# ---------------------------------------------------------------------------

DISCOVERY_PACKET = (
    b"MA\xff\xff\xff\xff\xff\xff\r\n"
    b"PW\r\n"
    b"MC\r\n"
    b"MN\r\n"
    b"VR\r\n"
    b"LI\r\n"
)


def discover(
    broadcast: str,
    port: int,
    timeout: float,
    *,
    log=sys.stdout,
) -> dict[str, tuple[str, dict[str, str]]]:
    """Broadcast a SEGCP discovery and return {mac: (src_ip, info)} dict."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", 0))
    sock.settimeout(0.2)

    print(
        f"[*] Discovering devices on {broadcast}:{port} "
        f"(host: {platform.system()}) ...",
        file=log,
    )
    try:
        sock.sendto(DISCOVERY_PACKET, (broadcast, port))
    except OSError as exc:
        print(f"[!] sendto failed: {exc}", file=sys.stderr)
        sock.close()
        return {}

    seen: dict[str, tuple[str, dict[str, str]]] = {}
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        info = parse_segcp_reply(data)
        if info is None:
            continue
        if info["mac"] not in seen:
            seen[info["mac"]] = (addr[0], info)

    sock.close()
    return seen


# ---------------------------------------------------------------------------
# DHCP SET command (unicast to a specific device)
# ---------------------------------------------------------------------------

def set_dhcp(
    device_ip: str,
    device_mac: str,
    password: str,
    port: int,
    *,
    recv_timeout: float = 1.0,
    log=sys.stdout,
) -> bool:
    """Send IM=1 / SV / RT to a single device. Returns True if packet was sent."""
    mac_bytes = mac_str_to_bytes(device_mac)
    # Targeting the device's real MAC grants SEGCP WRITE privilege,
    # which is required to SET IM, SV and RT (see segcp.c).
    packet = build_segcp_packet(mac_bytes, password, ["IM1", "SV", "RT"])

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", 0))
    sock.settimeout(recv_timeout)

    print(f"[*] Sending DHCP SET to {device_ip} (MAC {device_mac}) ...", file=log)
    try:
        sock.sendto(packet, (device_ip, port))
        # The device may or may not reply before rebooting — we try to read
        # the reply but do not fail if it doesn't arrive.
        try:
            data, _ = sock.recvfrom(4096)
            info = parse_segcp_reply(data)
            if info:
                im = info.get("IM", "?")
                print(
                    f"[+] Reply from {device_mac}: IM={im} "
                    f"({'DHCP' if im == '1' else 'static' if im == '0' else im})",
                    file=log,
                )
            else:
                print(f"[~] No readable reply from {device_mac} (device may be rebooting).", file=log)
        except socket.timeout:
            print(f"[~] No reply from {device_mac} (device may be rebooting).", file=log)
        return True
    except OSError as exc:
        print(f"[!] sendto to {device_ip} failed: {exc}", file=sys.stderr)
        return False
    finally:
        sock.close()


# ---------------------------------------------------------------------------
# Table formatting
# ---------------------------------------------------------------------------

def format_table(rows: list[tuple[str, str, str]], title: str) -> str:
    headers = ("MAC", "IP", "Version")
    widths = [max(len(h), *(len(r[i]) for r in rows)) for i, h in enumerate(headers)]
    fmt = "  ".join(f"{{:<{w}}}" for w in widths)
    lines = [title, fmt.format(*headers), fmt.format(*("-" * w for w in widths))]
    lines.extend(fmt.format(*r) for r in rows)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Networking hint
# ---------------------------------------------------------------------------

def warn_if_wsl() -> None:
    try:
        release = Path("/proc/sys/kernel/osrelease").read_text().lower()
    except OSError:
        return
    if "microsoft" not in release and "wsl" not in release:
        return
    print(
        "[!] WSL detected. With the default NAT networking mode, UDP\n"
        "    broadcasts are NOT forwarded to the LAN — no device will\n"
        "    reply. Either set `networkingMode=mirrored` in .wslconfig\n"
        "    and `wsl --shutdown`, or run this script from Windows.",
        file=sys.stderr,
    )


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Configure WIZ750SR module(s) to obtain their IP via DHCP.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  %(prog)s --all\n"
            "  %(prog)s --mac AA:BB:CC:DD:EE:FF\n"
            "  %(prog)s --mac AA:BB:CC:DD:EE:FF 11:22:33:44:55:66 -B 192.168.1.255\n"
        ),
    )

    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument(
        "-a", "--all", action="store_true",
        help="Configure every device that responds to the discovery broadcast.",
    )
    target.add_argument(
        "-m", "--mac", nargs="+", type=normalize_mac, metavar="MAC",
        help="Configure only the listed device(s) (e.g. AA:BB:CC:DD:EE:FF).",
    )

    parser.add_argument(
        "-B", "--broadcast", default="255.255.255.255",
        help="Broadcast address for discovery (default: 255.255.255.255). "
             "Use 192.168.x.255 to scope to one subnet.",
    )
    parser.add_argument(
        "-t", "--timeout", type=float, default=2.0,
        help="Seconds to wait for discovery replies (default: 2.0).",
    )
    parser.add_argument(
        "-p", "--port", type=int, default=SEGCP_PORT,
        help=f"SEGCP UDP port (default: {SEGCP_PORT}).",
    )
    parser.add_argument(
        "-P", "--password", default="",
        help="Device search password (default: empty).",
    )
    parser.add_argument(
        "-w", "--reboot-wait", type=float, default=8.0,
        help="Seconds to wait for devices to reboot before the final "
             "discovery (default: 8.0).",
    )

    args = parser.parse_args()

    warn_if_wsl()

    # --- Phase 1: discover ---
    seen = discover(args.broadcast, args.port, args.timeout)
    if not seen:
        print("[!] No devices found during initial discovery.", file=sys.stderr)
        return 1
    rows_before = sorted(
        [(mac, info.get("LI", "?"), info.get("VR", "?")) for mac, (_ip, info) in seen.items()],
        key=lambda r: r[0],
    )
    print()
    print(format_table(rows_before, f"[*] {len(seen)} device(s) found:"))

    # --- Resolve targets ---
    if args.all:
        targets = dict(seen)
    else:
        targets = {}
        missing = []
        for mac in args.mac:
            if mac in seen:
                targets[mac] = seen[mac]
            else:
                missing.append(mac)
        if missing:
            print(
                f"[!] The following MAC(s) were not found on the network:\n"
                + "\n".join(f"    {m}" for m in missing),
                file=sys.stderr,
            )
            if not targets:
                return 1

    # --- Phase 2: set DHCP on each target ---
    print(f"\n[*] Configuring {len(targets)} device(s) to use DHCP ...")
    ok_macs: list[str] = []
    for mac, (ip, _info) in targets.items():
        sent = set_dhcp(ip, mac, args.password, args.port)
        if sent:
            ok_macs.append(mac)

    if not ok_macs:
        print("[!] Failed to send DHCP command to any device.", file=sys.stderr)
        return 1

    # --- Phase 3: wait for reboot ---
    print(
        f"\n[*] Waiting {args.reboot_wait:.0f}s for device(s) to reboot "
        "and acquire DHCP leases ..."
    )
    time.sleep(args.reboot_wait)

    # --- Phase 4: re-discover and show results ---
    print()
    after = discover(args.broadcast, args.port, args.timeout)
    if not after:
        print("[!] No devices responded after reboot.", file=sys.stderr)
        return 1

    rows_after = sorted(
        [
            (mac, info.get("LI", "?"), info.get("VR", "?"))
            for mac, (_ip, info) in after.items()
        ] + [
            (mac, "(not seen)", "")
            for mac in ok_macs
            if mac not in after
        ],
        key=lambda r: r[0],
    )
    print(format_table(rows_after, f"[+] {len(after)} device(s) visible after reboot:"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
