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

After `--erase mass`, the data flash sector storing the MAC is wiped. On the next boot, use [`wiz750sr_setmac.py`](#wiz750sr_setmacpy) to write a new MAC; otherwise the Boot will block at the `INPUT FIRST MAC?` prompt.

---

## `wiz750sr_setmac.py`

Writes the MAC address of a freshly-flashed (or mass-erased) module.

### When you need it

`wiz750sr_flash.py --erase mass` wipes the data flash sector at
`0x0003FE00` (DAT0) that stores the MAC. On the next boot, the Boot
partition detects the missing MAC, prints `INPUT FIRST MAC?` on the
UART, and blocks waiting for a SEGCP `MC` command before continuing.

This script automates that exchange.

### Hardware setup

1. **BOOT pin LOW** (so the chip boots into Boot+App, *not* the ROM
   ISP) — opposite of what's required for `wiz750sr_flash.py`.
2. UART connected to the host (same wiring as for flashing).
3. Reset the module **after** the script starts — the boot prompt is
   printed once at startup and the script needs to be listening.

### Usage

```bash
python3 scripts/wiz750sr_setmac.py <MAC> -p <port> [options]
```

| Flag | Default | Description |
|---|---|---|
| `mac` | *(required)* | Target MAC address, e.g. `AA:BB:CC:DD:EE:FF`. Separators `:`, `-`, `.` accepted. |
| `-p`, `--port` | *(required)* | Serial port. |
| `-b`, `--baud` | `115200` | Boot's default UART baudrate. |
| `--no-wait` | — | Skip waiting for `INPUT FIRST MAC?` and send the command immediately. Use only if the prompt is already on screen. |

Example:

```bash
python3 scripts/wiz750sr_setmac.py 00:08:DC:12:34:56 -p /dev/ttyUSB0
```

Typical output:

```
[*] Port      : /dev/ttyUSB0 @ 115200 8N1
[*] MAC       : 00:08:DC:12:34:56
[*] Reminder  : BOOT pin must be LOW; reset the module AFTER this script starts.
[*] Waiting for 'INPUT FIRST MAC?' (reset the module now) ...
[+] Boot is waiting for MAC.
    | INPUT FIRST MAC?
[*] Sending   : b'MC00:08:DC:12:34:56\r\n'
[*] Reading boot output for verification ...
    | >> Firmware version: Boot 1.5.0 Develop
    | >> Network configuration: ...
[+] MAC 00:08:DC:12:34:56 written. Factory config restored.
```

### Notes and limits

- **One-shot write**: once a MAC is in DAT0, the App's `MC` SEGCP setter
  refuses to overwrite it if it starts with `00:08:DC` (WIZnet OUI
  factory protection, [boot/segcp.c:462](../s2e/boot/src/Configuration/segcp.c#L462)).
  To re-assign a MAC you must mass-erase and reflash first.
- The Boot first-boot prompt does **not** apply that protection, so any
  valid MAC (including a fresh `00:08:DC:xx:xx:xx` or a locally-
  administered MAC with the LAA bit set) is accepted here.
- The command sent is exactly 21 bytes (`MC<17-char MAC>\r\n`), which
  matches the Boot's blocking `S_UartGetc()` loop in
  [s2e/boot/src/main.c:514](../s2e/boot/src/main.c#L514).

---

## `wiz750sr_discover.py`

Discovers all WIZ750SR (and compatible WIZnet S2E) devices reachable on
the LAN via a SEGCP UDP broadcast — the same mechanism the WIZnet
config tool uses.

### How it works

Devices listen on UDP/50001 and respond to a broadcast query that
starts with the SEGCP `MA` command targeting the broadcast MAC
(`FF:FF:FF:FF:FF:FF`), which grants the sender read-only privilege.
The reply echoes the device's real MAC plus whatever GET commands were
appended to the query. The script asks for `MC` (MAC), `MN` (name),
`VR` (firmware version), `LI` (configured IP) and `OP` (working mode).

### Usage

```bash
python3 scripts/wiz750sr_discover.py [-B <bcast>] [-t <seconds>] [-p <port>]
```

| Flag | Default | Description |
|---|---|---|
| `-B`, `--broadcast` | `255.255.255.255` | Broadcast address. Use `192.168.x.255` to scope to a specific subnet (useful when the host has multiple interfaces). |
| `-t`, `--timeout` | `2.0` | Seconds to listen for replies. |
| `-p`, `--port` | `50001` | SEGCP UDP port. |
| `-s`, `--script` | — | Machine-readable TSV on stdout (no header, no decorations); diagnostics go to stderr. Exit 0 if ≥ 1 device found, 1 otherwise. |

Example output:

```
[*] Sending SEGCP discovery to 255.255.255.255:50001 (source port 50515, host: Linux)
[*] Listening for replies for 2.0s ...
[+] 2 device(s) found:

MAC                Source IP        Name              Version  Configured IP    Mode
-----------------  ---------------  ----------------  -------  ---------------  -----------
00:08:DC:12:34:56  192.168.11.42    WIZ750SR-bench    1.5.0    192.168.11.42    TCP server
00:08:DC:AA:BB:CC  192.168.11.57    WIZ750SR-test     1.5.0    192.168.11.57    TCP mixed
```

### Script-friendly mode

`-s` / `--script` emits one device per line, tab-separated, no header
or decorations. Field order: `mac<TAB>src_ip<TAB>name<TAB>version<TAB>local_ip<TAB>mode`.

Examples:

```bash
# Extract just the MACs:
python3 scripts/wiz750sr_discover.py -s | cut -f1

# Extract just the IPs:
python3 scripts/wiz750sr_discover.py -s | cut -f2

# Find the IP of a device by name:
python3 scripts/wiz750sr_discover.py -s | awk -F'\t' '$3=="WIZ750SR-bench" {print $2}'

# Loop over discovered devices:
python3 scripts/wiz750sr_discover.py -s | while IFS=$'\t' read -r mac ip name ver lip mode; do
    echo "$name @ $ip ($mac)"
done

# Suppress diagnostics entirely:
python3 scripts/wiz750sr_discover.py -s 2>/dev/null
```

### WSL2 caveat

WSL2's default NAT networking mode **does not forward UDP broadcasts to
the LAN**, so no device will reply. The script auto-detects WSL and
prints a warning. Workarounds:

1. **Mirrored networking** (recommended) — WSL2 ≥ 2.0.0 + Windows 11 22H2+.
   In `%USERPROFILE%\.wslconfig`:
   ```ini
   [wsl2]
   networkingMode=mirrored
   ```
   Then `wsl --shutdown` from PowerShell.
2. **Run from Windows directly** — install Python on Windows and run
   the script there. Same code works.

---

## `wiz750sr_setdhcp.py`

Configures one, several, or all WIZ750SR modules on the LAN to obtain
their IP address via DHCP — and prints the updated device list
(MAC, IP, version) once the modules have rebooted.

### How it works

1. **Discovery** — broadcast UDP SEGCP to find all reachable devices and
   display the initial table (MAC, current IP, version).
2. **Configuration** — for each target device, sends a unicast UDP SEGCP
   packet targeting the device's real MAC (which grants WRITE privilege):
   - `IM1` — IP address method = DHCP (0 = static, 1 = DHCP)
   - `SV` — save settings to flash
   - `RT` — reboot
3. **Re-discovery** — waits for the reboot delay, then broadcasts again
   and prints the final table with the newly assigned DHCP IPs.

Both tables are sorted by MAC address (ascending).

### Usage

```bash
python3 scripts/wiz750sr_setdhcp.py (-a | -m <MAC> [<MAC> ...]) [options]
```

One of `-a` / `-m` is required.

| Flag | Default | Description |
|---|---|---|
| `-a`, `--all` | — | Configure every device that responds to discovery. |
| `-m MAC [...]`, `--mac MAC [...]` | — | Configure only the listed device(s) (e.g. `AA:BB:CC:DD:EE:FF`). Separators `:`, `-`, `.` accepted. |
| `-B`, `--broadcast` | `255.255.255.255` | Broadcast address for discovery. Use `192.168.x.255` to scope to one subnet. |
| `-t`, `--timeout` | `2.0` | Seconds to wait for discovery replies. |
| `-p`, `--port` | `50001` | SEGCP UDP port. |
| `-P`, `--password` | empty | Device search password (only if configured). |
| `-w`, `--reboot-wait` | `8.0` | Seconds to wait for devices to reboot and acquire DHCP leases before the final discovery. |

### Examples

```bash
# Configure all devices on the LAN
python3 scripts/wiz750sr_setdhcp.py --all

# Configure a single device
python3 scripts/wiz750sr_setdhcp.py --mac 00:08:DC:12:34:56

# Configure two devices on a specific subnet
python3 scripts/wiz750sr_setdhcp.py -m 00:08:DC:12:34:56 00:08:DC:AA:BB:CC -B 192.168.1.255
```

Typical output:

```
[*] Discovering devices on 255.255.255.255:50001 (host: Linux) ...

[*] 2 device(s) found:
MAC                IP               Version
-----------------  ---------------  -------
00:08:DC:12:34:56  192.168.11.42    1.5.0
00:08:DC:AA:BB:CC  192.168.11.57    1.5.0

[*] Configuring 2 device(s) to use DHCP ...
[*] Sending DHCP SET to 192.168.11.42 (MAC 00:08:DC:12:34:56) ...
[~] No reply from 00:08:DC:12:34:56 (device may be rebooting).
[*] Sending DHCP SET to 192.168.11.57 (MAC 00:08:DC:AA:BB:CC) ...
[~] No reply from 00:08:DC:AA:BB:CC (device may be rebooting).

[*] Waiting 8s for device(s) to reboot and acquire DHCP leases ...

[*] Discovering devices on 255.255.255.255:50001 (host: Linux) ...
[+] 2 device(s) visible after reboot:
MAC                IP               Version
-----------------  ---------------  -------
00:08:DC:12:34:56  192.168.11.100   1.5.0
00:08:DC:AA:BB:CC  192.168.11.101   1.5.0
```

### Notes

- If a target device does not appear after the reboot wait, it is shown
  as `(not seen)` in the final table. Increase `-w` if your DHCP server
  is slow (the device must complete IP acquisition before answering
  SEGCP again).
- The WSL2 caveat from [`wiz750sr_discover.py`](#wiz750sr_discoverpy)
  applies equally here.

---

## `wiz750sr_netflash.py`

OTA-flash the **App** of a running WIZ750SR module over the network,
without touching the Boot. Reads the current version, performs the
flash, waits for the reboot, then reads the new version back.

### How it works

1. **BEFORE query** — TCP SEGCP to `<ip>:50001` with the broadcast MAC
   (READ privilege) to record current `VR` / `MC` / `MN`.
2. **FW SET** — second SEGCP TCP exchange targeting the device's real
   MAC (WRITE privilege) with `FW<size>`. The device replies
   `FW<ip>:50002\r\n`, closes the SEGCP socket, erases the App backup
   sector and opens a TCP listener on port 50002.
3. **Upload** — connects to `<ip>:50002` (with retries to ride out the
   erase delay) and streams the firmware bytes.
4. **Reboot wait** — polls the SEGCP port until the device replies
   again, then re-reads `VR`.

Only the App is flashed; the Boot remains untouched. The App backup
swap is handled by the Boot on the next reboot.

### Usage

```bash
python3 scripts/wiz750sr_netflash.py <app.bin> -i <ip> [options]
```

| Flag | Default | Description |
|---|---|---|
| `firmware` | *(required)* | App `.bin` to flash. Must be ≤ 100 KB (`DEVICE_FWUP_SIZE`). |
| `-i`, `--ip` | *(required)* | Device IP address. Find it with `wiz750sr_discover.py`. |
| `-m`, `--mac` | auto | Device MAC (needed for write privilege). Auto-discovered from the BEFORE query if omitted. |
| `-P`, `--password` | empty | Device search password (only if configured). |
| `--reboot-wait` | `20` | Seconds to wait for the device to come back online after the upload. |
| `-s`, `--script` | — | Machine-readable TSV on stdout, diagnostics on stderr. Exit 0 on success. |

Example (interactive):

```bash
$ python3 scripts/wiz750sr_netflash.py build/.../WIZ750SR_S2E_App.bin -i 192.168.11.42
[*] Firmware  : build/.../WIZ750SR_S2E_App.bin (87324 bytes)
[*] Target    : 192.168.11.42
[*] Querying device for current version ...
[*] Current   : version=1.5.0A mac=00:08:DC:12:34:56 name=WIZ750SR-bench
[*] Sending FW87324 (initiate OTA) ...
[*] Uploading to 192.168.11.42:50002 ...
[+] Upload OK (87324 bytes in 0.4s)
[*] Waiting up to 20s for reboot ...
[*] After     : version=1.5.1A
[+] OTA complete: 1.5.0A -> 1.5.1A
```

### Script-friendly mode

`-s` / `--script` outputs **one TSV line** on stdout:

```
<mac>\t<ip>\t<before_version>\t<after_version>\t<status>
```

`<status>` is one of:

| Status | Meaning |
|---|---|
| `OK` | Device rebooted and replied with a version |
| `OK_NO_VERSION` | Device rebooted but didn't return `VR` |
| `UNREACHABLE` | No reply to the initial BEFORE query |
| `NO_REPLY` | TCP open succeeded but reply wasn't a valid SEGCP frame |
| `FW_CMD_FAILED` | SEGCP TCP error while sending `FW<size>` |
| `FW_CMD_REJECTED` | Device didn't return `FW<ip>:<port>` (wrong MAC / password / size too big) |
| `UPLOAD_FAILED` | Could not connect to the OTA TCP port after retries |
| `UNREACHABLE_AFTER` | Upload OK but device never came back online |

Exit code: `0` if `<status> == OK*`, `1` otherwise, `2` for usage errors.

Examples:

```bash
# Just check success
wiz750sr_netflash.py app.bin -i 192.168.11.42 -s >/dev/null && echo "flashed"

# Pull just the new version
wiz750sr_netflash.py app.bin -i 192.168.11.42 -s | cut -f4

# Mass-flash a list of IPs and report
while read ip; do
    wiz750sr_netflash.py app.bin -i "$ip" -s
done < ips.txt | awk -F'\t' '{ printf "%-16s %-8s %s -> %s\n", $2, $5, $3, $4 }'

# Only the failures
wiz750sr_netflash.py app.bin -i 192.168.11.42 -s | grep -v $'\tOK\t' && echo "failure"
```

### Notes

- **Boot stays untouched.** Use `wiz750sr_flash.py` (UART/ISP) if you
  need to update the Boot too. For Boot+App updates over the network
  on a module already running this firmware, the SEGCP `BU` command
  would be the path — not implemented here.
- The reboot-wait timeout includes the App-backup → App-main swap, so
  20 s is generous for a healthy module. Increase it if your network
  has a slow DHCP server (the device must complete IP acquisition
  before answering SEGCP again).
- Multi-device runs: target one IP at a time. The device closes the
  SEGCP socket as soon as it accepts the `FW` command, so don't try
  to hold a long-running connection for batching.

---

### License

All scripts are released under the **MIT** license (see SPDX header in
each file), independently of the firmware license.
