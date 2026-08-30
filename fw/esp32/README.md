# WULPUS PRO ESP32 firmware

This ESP-IDF firmware connects the WULPUS PRO acquisition board to a PC through
either Wi-Fi/TCP or the native USB Serial/JTAG CDC interface of an ESP32-C6.
Both transports use the same framed binary protocol and can remain available
concurrently, while session arbitration ensures that only one host controls the
acquisition board at a time.

The **[WULPUS PRO WiFi host PCB](../../hw/wulpus_wifi_host_pcb)** is the primary
host board for this firmware. It contains a Seeed Studio XIAO ESP32-C6 and uses
the XIAO board configuration in this project. A standalone XIAO ESP32-C6 is
supported as a development alternative, and the ESP32-C6-DEVKITM-1 remains
supported for firmware bring-up.

The ESP32 receives fixed 804-byte RF frames from the MSP430 over 8 MHz SPI DMA,
buffers them without payload copies, and forwards them through the active PC
transport. Board access, acquisition, protocol processing, packet transmission,
and provisioning have separate components and FreeRTOS threads.

## Documentation

- [ESP-IDF toolchain setup](docs/toolchain.md) — installation on Windows,
  Linux, and macOS, VS Code configuration, environment activation, verification,
  and troubleshooting.
- [Firmware architecture](docs/architecture.md) — components, threads, data and
  control paths, DMA frame buffering, session lifecycle, and USB/Wi-Fi switching.
- [Wi-Fi provisioning](docs/provisioning.md) — first boot, SoftAP parameters,
  credential storage, reconnection, reprovisioning, and USB availability.
- [ESP32-to-MSP430 protocol](docs/msp_protocol.md) — SPI electrical settings,
  DATA_READY handshake, configuration-package fields, timing conversions,
  restart behavior, and RF frame layout.
- [ESP32-to-PC protocol](docs/esp_protocol.md) — USB/TCP framing, commands,
  acknowledgements, acquisition packets, status flags, and diagnostic counters.
- [Firmware changelog](CHANGELOG.md)

Python GUI and transport examples are in the repository's [`sw`](../../sw)
directory. The main interactive example is
[`wulpus_pro_wifi_example.ipynb`](../../sw/wulpus_pro_wifi_example.ipynb).

## Getting started

### Requirements

- ESP-IDF 6.0.1
- A WULPUS PRO WiFi host PCB or another supported ESP32-C6 board listed below
- A data-capable USB cable
- The matching WULPUS PRO MSP430 firmware on the acquisition board
- The ESP-IDF command-line environment or official ESP-IDF VS Code extension

The firmware is tested with:

- **WULPUS PRO WiFi host PCB containing a XIAO ESP32-C6 — primary host board**
- Standalone Seeed Studio XIAO ESP32-C6 — development alternative
- Espressif ESP32-C6-DEVKITM-1 — firmware bring-up/development board

Use a separate build directory and generated `sdkconfig` for each board.
ESP-IDF defaults initialize a new configuration but do not reliably replace
board-pin values already stored in an existing `sdkconfig`; isolated files avoid
accidentally building one board with another board's pinout.

### Install ESP-IDF

Install **ESP-IDF 6.0.1** and its ESP32-C6 toolchain before configuring the
firmware. Espressif's Installation Manager is the recommended method:

```text
eim install -i v6.0.1
```

Open an activated ESP-IDF terminal and verify:

```text
idf.py --version
```

See [ESP-IDF toolchain setup](docs/toolchain.md) for Windows, Linux, macOS,
VS Code, manual installation, and troubleshooting instructions.

### WULPUS PRO WiFi host PCB (primary)

The [WULPUS PRO WiFi host PCB](../../hw/wulpus_wifi_host_pcb) integrates the
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

### Standalone Seeed Studio XIAO ESP32-C6

A standalone XIAO ESP32-C6 uses the same `xiao-esp32c6.defaults`, build
commands, GPIO assignments, and WULPUS PRO connector mapping as the primary
WULPUS PRO WiFi host PCB. It is useful for firmware development or bench
testing when the integrated host PCB is not required.

### ESP32-C6-DEVKITM-1

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

### First connection

After flashing, reset the board and close any ESP-IDF monitor or serial terminal
that owns the COM port. The application console is disabled in the normal
ESP32-C6 configuration because USB CDC carries binary WULPUS protocol traffic.

For a wired connection to the WULPUS PRO WiFi host PCB or another supported
board, select **USB CDC** in the Python GUI, scan for devices, and open the
ESP32-C6 COM port. USB CDC does not use the selected UART baud rate as a
physical link-speed setting.

For Wi-Fi, an unprovisioned board automatically starts its SoftAP provisioning
service. Complete [Wi-Fi provisioning](docs/provisioning.md), then select
**Wi-Fi** in the GUI and scan for the mDNS-advertised device. Close an active USB
protocol session before attempting to control the board over Wi-Fi; the USB
cable itself may remain connected.

Do not use `idf.py flash monitor` for normal acquisition. A serial monitor keeps
the CDC port open and prevents the Python GUI from claiming it. Run a monitor
only for a deliberately configured debug build and close it before using USB
CDC protocol communication.

## Authors

- Sergei Vostrikov
- Cedric Hirschi, ETH Zurich

## License

Project source files are licensed under the terms stated in their headers and
the repository license files. ESP-IDF and its third-party components retain
their respective licenses.
