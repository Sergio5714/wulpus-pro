# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 

### Fixed

- Matched the XIAO ESP32-C6 defaults to the WULPUS PRO Wi-Fi host PCB routing.

### Changed

- Replaced the dedicated host-ready handshake with MSP430 reset gating: the ESP32 now holds the MSP430 in reset until a TCP client connects and asserts reset again after disconnects or communication failures.
- Added a bounded graceful-shutdown handshake that sends the MSP430 restart command and waits for its post-`disableAll()` configuration request before asserting reset, with immediate reset retained as a timeout/error fallback.

### Removed

- Removed the dedicated host-ready GPIO configuration and output handling.

## [1.0.0] - 2026-07-11

### Added

- Added ESP32-C6 Wi-Fi firmware for using an ESP32 module as a WULPUS PRO host interface.
- Added support for the Seeed Studio XIAO ESP32-C6 board.
- Added board-specific defaults for the Seeed Studio XIAO ESP32-C6 and ESP32-C6-DEVKITM-1.
- Added Wi-Fi provisioning support for storing network credentials in non-volatile memory.
- Added mDNS-based local network discovery.
- Added MSP430 reset control from the ESP32 host firmware.
- Added power-management support for light-sleep operation when idle.
- Added WULPUS PRO connector pin mapping documentation for supported ESP32-C6 boards.
- Added Python-side Wi-Fi usage documentation through `../../sw/wulpus_pro_wifi_example.ipynb`.

### Fixed

- Fixed the ESP32-C6-DEVKITM-1 defaults so `MSP_RST_N` no longer conflicts with SPI MOSI on GPIO2.

### Changed

- Updated the project to ESP-IDF 6.0.1.
- Refactored ESP-IDF configuration into reusable project defaults, target-specific defaults, and board-specific defaults.
- Documented first-time Wi-Fi provisioning after flashing the firmware.
- Documented tested board variants, board selection, build, flash, and serial-port usage.

### Removed

- Removed the old ESP32-C6-DEVKITM-1 reset mapping from GPIO2; `MSP_RST_N` is now mapped to GPIO3 for that board.
