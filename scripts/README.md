# scripts/

Host-side tooling for the WIZ750SR (W7500 / W7500P) project.

## `wiz750sr_flash.py`

Flashes a WIZ750SR module over UART using the W7500's **on-chip ISP
bootloader**. A cross-platform Python drop-in replacement for the
Windows-only proprietary tool shipped by WIZnet — tailored to the
combined Boot + App image produced by this project
(`build/.../s2e/firmware/WIZ750SR_S2E_Firmware.bin`).

---

### Software requirements

| Component | Version | Install |
|---|---|---|
| Python | ≥ 3.7 | `python3 --version` |
| `pyserial` | any | `pip install pyserial` (the PyPI package is `pyserial`, but the import stays `import serial`) |

No other dependencies — only the stdlib (`argparse`, `pathlib`, `re`, `time`…) is used.

#### Serial port access (Linux)

The user must belong to the `dialout` group (Debian/Ubuntu) or `uucp`
group (Arch):

```bash
sudo usermod -aG dialout $USER
# Log out / log back in for the group change to take effect
```

Otherwise: run under `sudo`, or adjust the device permissions
(`/dev/ttyUSB0`, `/dev/ttyACM0`).

#### WSL2 specific

WSL2 does not natively see Windows USB ports. Install
[`usbipd-win`](https://github.com/dorssel/usbipd-win) on the Windows
side:

```powershell
# Elevated PowerShell, on the Windows side
usbipd list
usbipd bind   --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

The device then shows up as `/dev/ttyUSB0` on the WSL side.

---

### Hardware requirements

1. **BOOT pin HIGH** *before* resetting the W7500.
   - WIZ750SR-EVB: `BOOT0` slide switch in the HIGH position.
   - Bare module: jumper the BOOT pin manually.
2. **Reset / power-cycle** so the chip enters the ROM ISP.
3. Module UART wired to the host (USB-UART adapter, e.g. the CP2104
   onboard the EVB).

⚠️ To reboot into application mode after flashing: set **BOOT LOW** then
reset (the script sends `REST`, but it will be ineffective if BOOT is
still HIGH — the chip will fall back into the ISP).

---

### Usage

```bash
python3 scripts/wiz750sr_flash.py <firmware.bin> -p <port> [options]
```

#### Options

| Flag | Default | Description |
|---|---|---|
| `firmware` | *(required)* | Path to the `.bin` to flash. |
| `-p`, `--port` | *(required)* | Serial port, e.g. `/dev/ttyUSB0`, `COM3`. |
| `-b`, `--baud` | `115200` | Baudrate. The W7500 ISP supports 2400–460800. |
| `-e`, `--erase` | `chip` | Erase mode: `chip`, `mass` (also erases data flash), `none`. |
| `--no-reset` | — | Skip the `REMP FLSH` + `REST` sequence after programming. |

#### Example

```bash
python3 scripts/wiz750sr_flash.py \
    build/gcc-arm-debug/s2e/firmware/WIZ750SR_S2E_Firmware.bin \
    -p /dev/ttyUSB0
```

Typical output:

```
[*] Firmware  : build/gcc-arm-debug/s2e/firmware/WIZ750SR_S2E_Firmware.bin (117484 bytes)
[*] Version   : Boot 1.5.0 Develop
[*] Version   : App  1.5.0 Develop
[*] Port      : /dev/ttyUSB0 @ 115200 8N1
[*] Reminder  : BOOT pin must be HIGH and the W7500 reset *before* this runs.
[*] Synchronising with bootloader ...
[+] Bootloader synchronised.
[*] ERAS CHIP
[*] XPRG 00000000 0001CB80  (117484 bytes payload)
  ... XMODEM 918/918 blocks
[*] REMP FLSH
[*] REST
[+] Done.
```

---

### Firmware version reported before flashing

The script reads an ASCII marker embedded in the `.bin` with the format:

```
WIZFWVER:<major>.<minor>.<maintenance>:<status>:END
```

This marker is defined in [s2e/common/src/fw_version.c](../s2e/common/src/fw_version.c)
and kept in flash via `KEEP(*(.fw_version))` in the linker scripts
(`s2e/boot/linker/sections.ld` and `s2e/app/linker/sections.ld`).

Because the `WIZ750SR_S2E_Common` library is linked into both Boot **and**
App, the combined firmware contains **two** markers (one per partition),
displayed on separate lines. To bump the version, edit the macros in
[s2e/common/include/common.h](../s2e/common/include/common.h) (`MAJOR_VER`,
`MINOR_VER`, `MAINTENANCE_VER`, `STR_VERSION_STATUS`); the marker is
regenerated automatically on the next build.

Quick shell-only check, without the script:

```bash
strings build/gcc-arm-debug/s2e/firmware/WIZ750SR_S2E_Firmware.bin | grep WIZFWVER
```

---

### W7500 ISP protocol (summary)

| Step | Host command | Device reply | Notes |
|---|---|---|---|
| Auto-baud sync | `'U'` (repeated) | `'U'` | Baud auto-detection, mandatory before any other command. |
| Post-sync purge | `\r` (bare CR) | `3\r\n` | **Required**: flushes residual state from auto-baud and confirms command mode. Without it, the next command returns `'3'` (invalid command). |
| Chip erase | `ERAS CHIP\r` | `0\r\n` | Erases code flash. |
| Mass erase | `ERAS MASS\r` | `0\r\n` | Also erases data flash. |
| Programming | `XPRG <addr8> <size8>\r` | *(no `0`)* | The chip emits `'C'` and switches to XMODEM/CRC receive mode. |
| Transfer | XMODEM, 128 B blocks, CRC-16/CCITT | `ACK` / `NAK` / `CAN` | Last block padded with `0x1A`, `EOT` on completion. |
| Remap | `REMP FLSH\r` | `0\r\n` | Activates the newly written image. |
| Reboot | `REST\r` | *(none)* | Hardware reset; no reply. |

All commands terminate with `\r`, all replies start with a numeric code
(`0` = OK) followed by `\r\n`.

---

### Troubleshooting

| Symptom | Likely cause |
|---|---|
| `ISP sync failed : no 'U' echo` | BOOT not HIGH at reset, wrong port/baud, or TX/RX swapped. |
| `XMODEM: bootloader never sent 'C'` | Erase taking too long, malformed `XPRG`, or chip not in ISP. |
| `XMODEM: block N not acknowledged after 16 retries` | Line noise ; try a lower baud (`-b 57600`). |
| `Permission denied: /dev/ttyUSB0` | See *Serial port access* section. |
| Module does not boot into application after flashing | BOOT still HIGH. Set it LOW and reset. |

Do not forget reset MAC at first boot if you erase all the memory (`--erase mass` option). 

---

### License

`wiz750sr_flash.py` is released under the **MIT** license (see SPDX
header in the file), independently of the firmware license.
