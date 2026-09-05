# WULPUS PRO ESP32 firmware

This ESP-IDF firmware connects the WULPUS PRO acquisition board to a PC through
either Wi-Fi/TCP or the native USB Serial/JTAG CDC interface of an ESP32-C6.
Both transports use the same framed binary protocol and can remain available
concurrently, while session arbitration ensures that only one host controls the
acquisition board at a time.

## Host board

The **[WULPUS PRO WiFi host PCB](../../hw/wulpus_wifi_host_pcb)** is the primary
host board for this firmware. It contains a Seeed Studio XIAO ESP32-C6 and uses
the XIAO board configuration in this project. A standalone XIAO ESP32-C6 is
supported as a development alternative, and the ESP32-C6-DEVKITM-1 remains
supported for firmware bring-up.

## Main functions

- **Runtime MSP430 configuration and ultrasound data acquisition:** sends
  acquisition settings and receives ultrasound frames through SPI with DMA.
- **Real-time data streaming over Wi-Fi or USB CDC:** selects the active
  transport at runtime through protocol sessions, with one controlling host
  at a time.
- **Persistent ESP32 configuration:** stores device boot policy, Wi-Fi settings,
  and credentials across power cycles; changes take effect after reboot.
- **MSP430 flashing:** accepts firmware images over USB/TCP and programs the
  MSP430FR5043 through four-wire JTAG.
- **Wi-Fi provisioning and discovery:** supports SoftAP setup, automatic
  reconnection, and mDNS discovery by the host application.
- **Buffering and diagnostics:** buffers acquisition frames and reports SPI,
  transport, and buffer errors, frame counters, and MSP430 update results.

## Getting started

### How to use (quick start)

Use a host board with this ESP32 firmware installed and an acquisition board
running the matching MSP430 firmware. For an unprogrammed ESP32, follow
[Development setup](docs/development.md). MSP430 installation is covered
in the separate [update guide](docs/msp430_update.md).

1. Connect the host board to the acquisition board and to the PC with a USB
   data cable. Close serial monitors and other applications using its COM port.
2. Follow the [Python software setup](../../sw/README.md) and open
   [`wulpus_pro_example.ipynb`](../../sw/wulpus_pro_example.ipynb).
3. Select **USB CDC** in the acquisition GUI, scan for devices, and open the
   ESP32-C6 port. For **Wi-Fi**, first complete
   [provisioning](docs/provisioning.md), then select Wi-Fi and discover the
   device in the GUI.
4. Set and apply the acquisition configuration in the notebook, then start
   acquisition in the GUI. Stop acquisition and close the active connection
   before switching between USB and Wi-Fi; the USB cable can remain connected.

The notebook also provides persistent ESP32 device configuration over USB,
including Wi-Fi boot policy and credentials. These changes take effect after
reboot. With the default policy, an unprovisioned board starts SoftAP
provisioning automatically; saved settings can disable it.

USB CDC carries the binary acquisition protocol, so the selected serial baud
rate does not set its physical transfer speed. Keep serial monitors closed
while using the GUI.

## Documentation

- [Development setup](docs/development.md) ? requirements, board configuration,
  building, flashing, and creating a merged firmware image.
- [MSP430 firmware updates](docs/msp430_update.md) — JTAG wiring, image upload,
  reboot-time programming, recovery, and firmware container format.
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
[`wulpus_pro_example.ipynb`](../../sw/wulpus_pro_example.ipynb).

## Authors

- Sergei Vostrikov
- Cedric Hirschi, ETH Zurich

## License

Project source files are licensed under the terms stated in their headers and
the repository license files. ESP-IDF and its third-party components retain
their respective licenses.
