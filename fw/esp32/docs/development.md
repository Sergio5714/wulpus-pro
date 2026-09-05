# ESP32 firmware development setup

Install the toolchain and build from source when developing firmware or
preparing a board that does not already have firmware installed.

## Requirements

- ESP-IDF 6.0.1
- A WULPUS PRO WiFi host PCB or another supported ESP32-C6 board listed below
- A data-capable USB cable
- The ESP-IDF command-line environment or official ESP-IDF VS Code extension

The acquisition board configurations are:

- **WULPUS PRO WiFi host PCB containing a XIAO ESP32-C6 — primary host board**
- Standalone Seeed Studio XIAO ESP32-C6 — development alternative
- Espressif ESP32-C6-DEVKITM-1 — firmware bring-up/development board

Use a separate build directory and generated `sdkconfig` for each board.
ESP-IDF defaults initialize a new configuration but do not reliably replace
board-pin values already stored in an existing `sdkconfig`; isolated files avoid
accidentally building one board with another board's pinout.

Run all `idf.py` commands below from `fw/esp32`. From the repository root:

```powershell
Set-Location fw/esp32
```

The commands shown use PowerShell continuation/quoting syntax. Existing
`sdkconfig.xiao` or `sdkconfig.devkit` files retain saved values; compare them
with the board defaults when updating.

## Install ESP-IDF

Install **ESP-IDF 6.0.1** and its ESP32-C6 toolchain before configuring the
firmware. Espressif's Installation Manager is the recommended method:

```text
eim install -i v6.0.1
```

Open an activated ESP-IDF terminal and verify:

```text
idf.py --version
```

See [ESP-IDF toolchain setup](toolchain.md) for Windows, Linux, macOS,
VS Code, manual installation, and troubleshooting instructions.

## WULPUS PRO WiFi host PCB (primary)

The [WULPUS PRO WiFi host PCB](../../../hw/wulpus_wifi_host_pcb) integrates the
XIAO ESP32-C6 with the WULPUS PRO host connector and is the recommended board
for normal use. Configure it with the XIAO defaults:

```powershell
idf.py -B build-xiao `
  -D SDKCONFIG=sdkconfig.xiao `
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32c6;boards/xiao-esp32c6.defaults" `
  reconfigure
```

Build and flash, replacing `COM10` with the port assigned by Windows:

```powershell
idf.py -B build-xiao build
idf.py -B build-xiao -p COM10 flash
```

Use the full `flash` command above when upgrading older firmware so the
bootloader, partition table, and application are installed together.

For a single image to distribute to a fresh ESP32, run
`idf.py -B build-xiao merge-bin` after building. Flash the resulting
`build-xiao/merged-binary.bin` at `0x0`. The application
`wulpus-pro-fw.bin` alone does not include the bootloader or partition table.

WULPUS PRO WiFi host PCB routing:

| Signal | ESP32-C6 GPIO | XIAO pin | WULPUS PRO connector |
|---|---:|---|---|
| `SPI_SS` | 17 | D7 | X3.4 |
| `SPI_CLK` | 18 | D10 | X3.3 |
| `SPI_MISO` | 20 | D9 / MISO | X3.1 |
| `SPI_MOSI` | 19 | D8 / SCK | X3.2 |
| `DATA_READY` | 1 | D1 | X4.2 |
| `MSP_RST_N` | 0 | D0 | X1.5 |

The WULPUS PRO WiFi host PCB routing intentionally differs from the standard
SPI function labels printed for the XIAO header. Follow the GPIO and connector
assignments above.

## Standalone Seeed Studio XIAO ESP32-C6

A standalone XIAO ESP32-C6 uses the same `xiao-esp32c6.defaults`, build
commands, GPIO assignments, and WULPUS PRO connector mapping as the primary
WULPUS PRO WiFi host PCB. It is useful for firmware development or bench
testing when the integrated host PCB is not required.

## ESP32-C6-DEVKITM-1

Configure an isolated DevKit build:

```powershell
idf.py -B build-devkit `
  -D SDKCONFIG=sdkconfig.devkit `
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32c6;boards/esp32c6-devkitm-1.defaults" `
  reconfigure
```

Build and flash:

```powershell
idf.py -B build-devkit build
idf.py -B build-devkit -p COM10 flash
```

DevKit wiring:

| Signal | ESP32-C6 GPIO | DevKit header | WULPUS PRO connector |
|---|---:|---|---|
| `SPI_SS` | 18 | 18 | X3.4 |
| `SPI_CLK` | 6 | 6 | X3.3 |
| `SPI_MISO` | 7 | 7 | X3.1 |
| `SPI_MOSI` | 2 | 2 | X3.2 |
| `DATA_READY` | 1 | 1/N | X4.2 |
| `MSP_RST_N` | 3 | 3 | X1.5 |

GPIO2 is used for `SPI_MOSI`, so the DevKit defaults place `MSP_RST_N` on
GPIO3.

After flashing, reset the ESP32 and follow
[How to use (quick start)](../README.md#how-to-use-quick-start). The normal configuration
disables the application console because USB CDC carries binary protocol data.
Use a serial monitor only with a deliberately configured debug build, and close
it before opening the acquisition GUI.

