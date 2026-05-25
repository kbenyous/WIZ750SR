#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
Discover WIZ750SR (W7500 / W7500P) devices on the LAN via SEGCP UDP
broadcast — the same mechanism the WIZnet config tool uses.

Protocol (from s2e/app/src/Configuration/segcp.c, proc_SEGCP_udp):
  - Devices listen on UDP/50001 (DEVICE_SEGCP_PORT).
  - A discovery packet starts with the SEGCP MA command targeting the
    broadcast MAC FF:FF:FF:FF:FF:FF, which grants the sender READ-only
    privilege. It is followed by PW (search password, empty if none),
    then any GET commands (MC, MN, VR, LI, ...).
  - The device replies via UDP broadcast (\xff\xff\xff\xff) to the
    sender's source port. The reply echoes "MA<6 bytes>\\r\\n" with the
    device's actual MAC, followed by "PW...\\r\\n", then one
    "<cmd><value>\\r\\n" entry per GET command in the original request.

Networking caveats:
  - WSL2's default NAT does NOT forward UDP broadcasts to the LAN. To
    use this script from WSL, switch to mirrored networking mode
    (`networkingMode=mirrored` in %USERPROFILE%\\.wslconfig) or run it
    from Windows directly.
  - On hosts with multiple interfaces, 255.255.255.255 may only reach
    one of them. Use -B <subnet>.255 to scope to a specific LAN.
"""

from __future__ import annotations

import argparse
import platform
import socket
import sys
import time
from pathlib import Path

SEGCP_PORT = 50001

# Built once: targets the broadcast MAC, no password, then asks for the
# fields most useful to identify a device.
DISCOVERY_PACKET = (
    b"MA\xff\xff\xff\xff\xff\xff\r\n"  # target = broadcast MAC -> READ privilege
    b"PW\r\n"                          # empty search password
    b"MC\r\n"                          # MAC as ASCII XX:XX:...
    b"MN\r\n"                          # module name
    b"VR\r\n"                          # firmware version
    b"LI\r\n"                          # configured local IP
    b"OP\r\n"                          # working mode (0=TCP cli, 1=TCP svr, 2=mixed, 3=UDP)
)


def parse_reply(data: bytes) -> dict[str, str] | None:
    """Decode a SEGCP reply into a flat dict, or None if it isn't one.

    Layout: 10 bytes binary header "MA<6 bytes>\\r\\n", then ASCII tokens
    separated by "\\r\\n" — first is the echoed PW, the rest are the
    "<2-char-cmd><value>" responses in the order they were requested.
    """
    if len(data) < 10 or not data.startswith(b"MA"):
        return None
    mac_bin = data[2:8]
    if mac_bin == b"\xff\xff\xff\xff\xff\xff":
        # Echo of our own broadcast; the device puts its REAL mac here.
        return None
    fields: dict[str, str] = {
        "mac": ":".join(f"{b:02X}" for b in mac_bin),
    }
    tail = data[10:].decode("ascii", errors="replace")
    for token in tail.split("\r\n"):
        if len(token) >= 2 and token[:2].isalpha():
            fields[token[:2]] = token[2:]
    return fields


def warn_if_wsl() -> None:
    """Print a hint if we're running inside WSL2 with default networking."""
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


OP_MODES = {"0": "TCP client", "1": "TCP server", "2": "TCP mixed", "3": "UDP"}


FIELD_ORDER = ("mac", "src_ip", "name", "version", "local_ip", "mode")


def row_for(src_ip: str, info: dict[str, str]) -> tuple[str, ...]:
    """Project a device record into the canonical FIELD_ORDER tuple."""
    return (
        info.get("mac", ""),
        src_ip,
        info.get("MN", ""),
        info.get("VR", ""),
        info.get("LI", ""),
        OP_MODES.get(info.get("OP", ""), info.get("OP", "")),
    )


def format_table(seen: dict[str, tuple[str, dict[str, str]]]) -> str:
    """Render the result set as a fixed-width human-readable table."""
    headers = ("MAC", "Source IP", "Name", "Version", "Configured IP", "Mode")
    rows = [row_for(src_ip, info) for _, (src_ip, info) in sorted(seen.items())]
    widths = [max(len(h), *(len(r[i]) for r in rows)) for i, h in enumerate(headers)]
    fmt = "  ".join(f"{{:<{w}}}" for w in widths)
    lines = [fmt.format(*headers), fmt.format(*("-" * w for w in widths))]
    lines.extend(fmt.format(*r) for r in rows)
    return "\n".join(lines)


def format_tsv(seen: dict[str, tuple[str, dict[str, str]]]) -> str:
    """Render the result set as TSV (no header, one device per line).

    Tabs / newlines inside fields are replaced by spaces so the format
    stays cut/awk-safe.
    """
    out = []
    for _, (src_ip, info) in sorted(seen.items()):
        safe = (f.replace("\t", " ").replace("\n", " ") for f in row_for(src_ip, info))
        out.append("\t".join(safe))
    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Discover WIZ750SR devices on the LAN via SEGCP UDP broadcast."
    )
    parser.add_argument(
        "-B", "--broadcast", default="255.255.255.255",
        help="Broadcast address (default: 255.255.255.255). "
             "Use 192.168.x.255 to scope the query to one subnet.",
    )
    parser.add_argument(
        "-t", "--timeout", type=float, default=2.0,
        help="Seconds to wait for replies after sending the query (default: 2.0).",
    )
    parser.add_argument(
        "-p", "--port", type=int, default=SEGCP_PORT,
        help=f"SEGCP UDP port (default: {SEGCP_PORT}).",
    )
    parser.add_argument(
        "-s", "--script", action="store_true",
        help="Machine-readable TSV output on stdout, no header, no decorations. "
             "Diagnostic messages still go to stderr. Field order: "
             "mac<TAB>src_ip<TAB>name<TAB>version<TAB>local_ip<TAB>mode.",
    )
    args = parser.parse_args()

    # In script mode, redirect human-friendly messages to stderr so stdout
    # only carries the parseable rows.
    log = sys.stderr if args.script else sys.stdout

    warn_if_wsl()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    # Some OSes (notably Windows) need SO_REUSEADDR when binding to receive
    # broadcasts on the same port the device targets on its reply.
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", 0))
    sock.settimeout(0.2)
    src_port = sock.getsockname()[1]

    print(
        f"[*] Sending SEGCP discovery to {args.broadcast}:{args.port} "
        f"(source port {src_port}, host: {platform.system()})",
        file=log,
    )
    try:
        sock.sendto(DISCOVERY_PACKET, (args.broadcast, args.port))
    except OSError as exc:
        print(f"[!] sendto failed: {exc}", file=sys.stderr)
        return 1

    print(f"[*] Listening for replies for {args.timeout:.1f}s ...", file=log)
    seen: dict[str, tuple[str, dict[str, str]]] = {}
    deadline = time.monotonic() + args.timeout
    while time.monotonic() < deadline:
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        info = parse_reply(data)
        if info is None:
            continue
        # Deduplicate by MAC (a device may reply once per matching iface).
        if info["mac"] not in seen:
            seen[info["mac"]] = (addr[0], info)

    sock.close()

    if not seen:
        print("[!] No devices responded.", file=sys.stderr)
        return 1

    if args.script:
        print(format_tsv(seen))
    else:
        print(f"[+] {len(seen)} device(s) found:")
        print()
        print(format_table(seen))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
