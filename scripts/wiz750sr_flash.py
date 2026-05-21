#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
Flash the WIZ750SR (W7500 / W7500P MCU) over UART using the on-chip ISP
bootloader. Drop-in replacement for the proprietary Windows-only WIZnet
ISP tool, suitable for the combined 'Boot+App' firmware image produced
by this project (build/.../s2e/firmware/W750SR_Firmware.bin).

Hardware setup before running:
  - Pull BOOT pin HIGH, then reset the W7500 (or power-cycle).
  - Connect the host to the module UART (USB-UART adapter, e.g. CP2104
    on the WIZ750SR-EVB). On the EVB the BOOT0 slide switch handles
    the BOOT pin; on bare modules, jumper it manually.

Protocol references (extracted from WIZnet/W7500_ISP):
  - Auto-baud sync : host sends 'U', device replies 'U'.
  - Commands : "<CMD>\\r"  responses : "<CODE>\\r\\n",  '0' = OK.
  - Erase     : "ERAS CHIP" (code) or "ERAS MASS" (code + data flash).
  - Program   : "XPRG <addr_hex8> <size_hex8>\\r" then a standard
                XMODEM (128-byte blocks, CRC-16/SOH) transfer.
  - Finalize  : "REMP FLSH" then "REST" to remap and reboot.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
import time
from pathlib import Path

import serial

# --- Firmware version marker --------------------------------------------------
#
# The firmware embeds a fixed-format ASCII string in flash via
# s2e/common/src/fw_version.c so this script can report which version is
# about to be flashed without needing source access or a running device.
# Format: "WIZFWVER:<major>.<minor>.<maintenance>:<status>:END"

_FW_VERSION_RE = re.compile(rb"WIZFWVER:([0-9.]+):([A-Za-z]+):END")

# In the combined firmware image (Boot at 0x0000, App at 0x7000) one marker
# lands in each partition. This threshold picks the partition label.
_APP_START_OFFSET = 0x7000


def extract_fw_versions(payload: bytes) -> list[tuple[str, str, str]]:
    """Return [(partition, version, status), ...] found in the image."""
    out = []
    for m in _FW_VERSION_RE.finditer(payload):
        partition = "Boot" if m.start() < _APP_START_OFFSET else "App"
        out.append(
            (partition, m.group(1).decode("ascii"), m.group(2).decode("ascii"))
        )
    return out

# --- XMODEM (128-byte blocks, CRC-16/CCITT) -----------------------------------

SOH, EOT, ACK, NAK, CAN = 0x01, 0x04, 0x06, 0x15, 0x18
CRC_REQ = ord("C")
BLOCK_SIZE = 128


def _crc16_ccitt(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def xmodem_send(ser: serial.Serial, payload: bytes, *, retries: int = 16) -> None:
    """Send `payload` using XMODEM/CRC. Pads the final block with 0x1A (SUB)."""
    # Wait for the receiver (the W7500 bootloader) to request CRC mode with 'C'.
    deadline = time.monotonic() + 60.0
    while True:
        c = ser.read(1)
        if c == bytes([CRC_REQ]):
            break
        if time.monotonic() > deadline:
            raise TimeoutError("XMODEM: bootloader never sent 'C' (CRC request)")

    total = (len(payload) + BLOCK_SIZE - 1) // BLOCK_SIZE
    for block_idx in range(total):
        chunk = payload[block_idx * BLOCK_SIZE : (block_idx + 1) * BLOCK_SIZE]
        if len(chunk) < BLOCK_SIZE:
            chunk = chunk + b"\x1A" * (BLOCK_SIZE - len(chunk))
        seq = (block_idx + 1) & 0xFF
        crc = _crc16_ccitt(chunk)
        frame = bytes([SOH, seq, 0xFF - seq]) + chunk + bytes([(crc >> 8) & 0xFF, crc & 0xFF])

        for attempt in range(retries):
            ser.write(frame)
            ser.flush()
            resp = ser.read(1)
            if resp == bytes([ACK]):
                break
            if resp == bytes([CAN]):
                raise IOError("XMODEM: receiver cancelled (CAN)")
            # NAK or timeout : retry
        else:
            raise IOError(f"XMODEM: block {seq} not acknowledged after {retries} retries")

        if (block_idx + 1) % 16 == 0 or block_idx + 1 == total:
            print(f"  ... XMODEM {block_idx + 1}/{total} blocks", end="\r", flush=True)

    print()
    # End-of-transmission handshake.
    for _ in range(retries):
        ser.write(bytes([EOT]))
        ser.flush()
        if ser.read(1) == bytes([ACK]):
            return
    raise IOError("XMODEM: EOT not acknowledged")


# --- ISP command layer --------------------------------------------------------


class W7500Isp:
    def __init__(self, port: str, baud: int, timeout: float = 1.0) -> None:
        self.ser = serial.Serial(port, baud, timeout=timeout)

    def close(self) -> None:
        if self.ser.is_open:
            self.ser.close()

    def sync(self, attempts: int = 20) -> None:
        """Auto-baud handshake : send 'U' until the bootloader echoes 'U',
        then send a bare CR and expect '3\\r\\n' (invalid command).

        The bare-CR step matches what WIZnet's official tool does after
        auto-baud (W7500_ISP.py: writeCmd("", "3")). It flushes any
        residual state from the 'U' negotiation and confirms the
        bootloader is in command mode. Without it, the first real
        command (e.g. ERAS CHIP) returns '3' instead of '0'.
        """
        self.ser.reset_input_buffer()
        for _ in range(attempts):
            self.ser.write(b"U")
            self.ser.flush()
            if self.ser.read(1) == b"U":
                # Drain any trailing banner the bootloader may send.
                time.sleep(0.05)
                self.ser.reset_input_buffer()
                # Post-sync purge : bare CR -> expect '3' (invalid cmd).
                self.ser.write(b"\r")
                self.ser.flush()
                resp = self.ser.readline().decode("ascii", errors="replace").strip()
                if not resp.startswith("3"):
                    raise IOError(
                        f"ISP post-sync purge failed : got {resp!r}, expected leading '3'"
                    )
                return
        raise IOError(
            "ISP sync failed : no 'U' echo from W7500. "
            "Check BOOT pin (must be HIGH at reset), wiring, and baudrate."
        )

    def cmd(self, command: str, *, expect: str = "0") -> str:
        """Send "<command>\\r" and read one "<resp>\\r\\n" line."""
        self.ser.write(command.encode("ascii") + b"\r")
        self.ser.flush()
        resp = self.ser.readline().decode("ascii", errors="replace").strip()
        if not resp.startswith(expect):
            raise IOError(f"ISP {command!r} failed : got {resp!r}, expected leading {expect!r}")
        return resp

    def erase(self, mode: str) -> None:
        if mode == "none":
            return
        cmd = "ERAS MASS" if mode == "mass" else "ERAS CHIP"
        print(f"[*] {cmd}")
        self.cmd(cmd)

    def program(self, payload: bytes, start_addr: int = 0x00000000) -> None:
        # Align reported size up to the XMODEM block boundary.
        size = (len(payload) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1)
        print(f"[*] XPRG {start_addr:08X} {size:08X}  ({len(payload)} bytes payload)")
        # XPRG does NOT reply with a status line ; the bootloader goes
        # straight into XMODEM-receive mode and emits 'C'.
        self.ser.write(f"XPRG {start_addr:08X} {size:08X}\r".encode("ascii"))
        self.ser.flush()
        xmodem_send(self.ser, payload)

    def finalize(self) -> None:
        print("[*] REMP FLSH")
        self.cmd("REMP FLSH")
        print("[*] REST")
        # REST does not return '0' : it just resets the chip. Best-effort write.
        self.ser.write(b"REST\r")
        self.ser.flush()


# --- CLI ----------------------------------------------------------------------


W7500_FLASH_SIZE = 128 * 1024  # 128 KB total flash on W7500


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Flash a WIZ750SR (W7500) over UART via the on-chip ISP bootloader."
    )
    parser.add_argument("firmware", type=Path, help="Path to the .bin firmware image to flash.")
    parser.add_argument(
        "-p", "--port", required=True,
        help="Serial port (e.g. /dev/ttyUSB0, /dev/ttyACM0, COM3).",
    )
    parser.add_argument(
        "-b", "--baud", type=int, default=115200,
        help="Baudrate (default: 115200). Supported by W7500 ISP: 2400..460800.",
    )
    parser.add_argument(
        "-e", "--erase", choices=("chip", "mass", "none"), default="chip",
        help="Erase mode before programming (default: chip).",
    )
    parser.add_argument(
        "--no-reset", action="store_true",
        help="Do not send REMP FLSH + REST after programming.",
    )
    args = parser.parse_args()

    if not args.firmware.is_file():
        print(f"error: firmware not found: {args.firmware}", file=sys.stderr)
        return 2
    payload = args.firmware.read_bytes()
    if not payload:
        print("error: firmware file is empty", file=sys.stderr)
        return 2
    if len(payload) > W7500_FLASH_SIZE:
        print(
            f"error: firmware size {len(payload)} > W7500 flash ({W7500_FLASH_SIZE} bytes)",
            file=sys.stderr,
        )
        return 2

    print(f"[*] Firmware  : {args.firmware} ({len(payload)} bytes)")
    versions = extract_fw_versions(payload)
    if versions:
        for partition, ver, status in versions:
            print(f"[*] Version   : {partition} {ver} {status}")
    else:
        print("[*] Version   : (no WIZFWVER marker found)")
    print(f"[*] Port      : {args.port} @ {args.baud} 8N1")
    print("[*] Reminder  : BOOT pin must be HIGH and the W7500 reset *before* this runs.")

    isp = W7500Isp(args.port, args.baud)
    try:
        print("[*] Synchronising with bootloader ...")
        isp.sync()
        print("[+] Bootloader synchronised.")
        isp.erase(args.erase)
        isp.program(payload)
        if not args.no_reset:
            isp.finalize()
        print("[+] Done.")
    except (IOError, TimeoutError) as exc:
        print(f"[!] {exc}", file=sys.stderr)
        return 1
    finally:
        isp.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
