#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
Set the MAC address of a freshly-flashed WIZ750SR module.

After a full chip erase (e.g. `wiz750sr_flash.py --erase mass`), the
data flash sector that stores the MAC (DAT0 @ 0x0003FE00) is wiped
(0xFF). On the next boot, the Boot partition detects the missing MAC
and prints "INPUT FIRST MAC?" on the UART, then blocks waiting for a
21-character SEGCP command:

    MC<AA:BB:CC:DD:EE:FF>\\r\\n

Once accepted, the Boot writes the MAC to data flash, restores
factory-default config, and continues booting normally.

Hardware setup before running:
  - BOOT pin LOW (so the chip boots into Boot+App, not the ROM ISP).
  - UART connected to the host (same wiring as for `wiz750sr_flash.py`).
  - Reset the module AFTER starting this script — the prompt is only
    printed once at boot.

Reference: s2e/boot/src/main.c (input_first_mac()) and
s2e/boot/src/Configuration/segcp.c (proc_SEGCP / SEGCP_MC).
"""

from __future__ import annotations

import argparse
import re
import sys
import time

import serial

PROMPT = b"INPUT FIRST MAC?"
PROMPT_TIMEOUT_S = 30.0   # how long to wait for the boot prompt
SETTLE_TIMEOUT_S = 5.0    # how long to read trailing boot output after write
MAC_RE = re.compile(r"^[0-9A-Fa-f]{2}([:\-.])(?:[0-9A-Fa-f]{2}\1){4}[0-9A-Fa-f]{2}$")


def normalize_mac(mac: str) -> str:
    """Validate input and return canonical 'AA:BB:CC:DD:EE:FF' form (uppercase)."""
    if not MAC_RE.match(mac):
        raise argparse.ArgumentTypeError(
            f"invalid MAC {mac!r}: expected 6 hex octets with ':' '-' or '.' separators"
        )
    octets = re.split(r"[:\-.]", mac)
    return ":".join(o.upper() for o in octets)


def wait_for_prompt(ser: serial.Serial, deadline: float) -> bytes:
    """Read until PROMPT appears in the stream. Return everything captured."""
    buf = bytearray()
    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if chunk:
            buf.extend(chunk)
            if PROMPT in buf:
                return bytes(buf)
    raise TimeoutError(
        f"never saw {PROMPT.decode()!r} prompt on UART within "
        f"{PROMPT_TIMEOUT_S:.0f}s. Captured so far:\n"
        f"  {bytes(buf).decode('ascii', 'replace')!r}"
    )


def drain(ser: serial.Serial, seconds: float) -> bytes:
    """Read everything that arrives within `seconds` and return it."""
    deadline = time.monotonic() + seconds
    buf = bytearray()
    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if chunk:
            buf.extend(chunk)
    return bytes(buf)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Write the MAC address of a freshly-flashed WIZ750SR module."
    )
    parser.add_argument(
        "mac", type=normalize_mac,
        help="MAC address to assign, e.g. AA:BB:CC:DD:EE:FF (':' '-' or '.' separators).",
    )
    parser.add_argument(
        "-p", "--port", required=True,
        help="Serial port (e.g. /dev/ttyUSB0, COM3).",
    )
    parser.add_argument(
        "-b", "--baud", type=int, default=115200,
        help="Baudrate (default: 115200, the Boot partition's default).",
    )
    parser.add_argument(
        "--no-wait", action="store_true",
        help="Skip waiting for 'INPUT FIRST MAC?' (send the command immediately). "
             "Use only if the prompt is already on screen.",
    )
    args = parser.parse_args()

    print(f"[*] Port      : {args.port} @ {args.baud} 8N1")
    print(f"[*] MAC       : {args.mac}")
    print("[*] Reminder  : BOOT pin must be LOW; reset the module AFTER this script starts.")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.2)
    except serial.SerialException as exc:
        print(f"[!] cannot open {args.port}: {exc}", file=sys.stderr)
        return 1

    try:
        ser.reset_input_buffer()

        if args.no_wait:
            print("[*] --no-wait: sending MC command immediately.")
        else:
            print(f"[*] Waiting for {PROMPT.decode()!r} (reset the module now) ...")
            try:
                captured = wait_for_prompt(ser, time.monotonic() + PROMPT_TIMEOUT_S)
            except TimeoutError as exc:
                print(f"[!] {exc}", file=sys.stderr)
                print(
                    "[!] If the MAC is already set, the prompt won't appear. "
                    "To re-set it you'd need to erase the data flash (DAT0 @ 0x3FE00)\n"
                    "    via `wiz750sr_flash.py --erase mass` and reflash first.",
                    file=sys.stderr,
                )
                return 1
            print("[+] Boot is waiting for MAC.")
            # Show whatever the boot printed before the prompt (useful for diagnostics).
            tail = captured.decode("ascii", "replace").splitlines()[-3:]
            for line in tail:
                if line.strip():
                    print(f"    | {line}")

        # Build the SEGCP MC command: "MCAA:BB:CC:DD:EE:FF\r\n" (21 bytes).
        cmd = f"MC{args.mac}\r\n".encode("ascii")
        assert len(cmd) == 21, f"command size {len(cmd)} != 21 (boot reads exactly 21 chars)"
        print(f"[*] Sending   : {cmd!r}")
        ser.write(cmd)
        ser.flush()

        # The boot writes the MAC, calls set_DevConfig_to_factory_value(),
        # save_DevConfig_to_storage(), then continues booting. Capture the
        # tail of that output so the user can confirm success.
        print("[*] Reading boot output for verification ...")
        tail = drain(ser, SETTLE_TIMEOUT_S).decode("ascii", "replace")
        if tail:
            for line in tail.splitlines():
                if line.strip():
                    print(f"    | {line}")
        # The boot re-prompts only if proc_SEGCP returned an error.
        if PROMPT in tail.encode("ascii", "replace"):
            print(
                "[!] Boot re-prompted: MAC write was rejected. Check format and retry.",
                file=sys.stderr,
            )
            return 1

        print(f"[+] MAC {args.mac} written. Factory config restored.")
        return 0
    finally:
        ser.close()


if __name__ == "__main__":
    raise SystemExit(main())
