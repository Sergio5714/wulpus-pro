# Changelog

All main changes to this project will be documented in this file.
For the detailed description, please explore nested folders and corresponding CHANGELOG.md files (e.g. for PCB projects or firmware). 

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added ESP32-C6 Wi-Fi firmware support for the Seeed Studio XIAO ESP32-C6 board, including board-specific defaults and README pin mapping.
- Added MSP430 board reset control logic to the ESP32 Wi-Fi firmware.
- Added a Python Wi-Fi example notebook for discovery, connection setup, configuration transfer, and data acquisition over the ESP32 TCP link.
- Added `uv` project metadata and lockfile for Python dependency management.
- Added README comparison table between WULPUS PRO and the original WULPUS platform.

### Fixed

- Fixed the MSP430 firmware initialization so the preamplifier power switch is always enabled at startup.
- Fixed the ESP32-C6-DEVKITM-1 board defaults to avoid reusing GPIO2 for both SPI MOSI and MSP430 reset.

### Changed

- Refactored the ESP32 firmware configuration around reusable ESP-IDF defaults, target-specific defaults, and board-specific pinout files.
- Updated the ESP32 project from ESP-IDF 5.4.1 to ESP-IDF 6.0.1.
- Updated ESP32 documentation for tested DevKit and XIAO boards, board selection, flashing, serial port selection, and WULPUS PRO connector pin mappings.
- Updated project READMEs for repository structure, build instructions, usage flows, firmware links, and WULPUS PRO-specific documentation pointers.
- Migrated Python package management from conda to uv.
- Updated README full specifications with grouped system, receive-chain, data-link, imaging, power, and mechanics values.
- Updated gitignore rules.

### Removed

- Removed the old ESP32-C6-DEVKITM-1 MSP430 reset mapping from GPIO2; GPIO2 is used for SPI MOSI and MSP430 reset is now on GPIO3.
- Removed the old Conda `sw/requirements.yml` dependency file.

## [0.1.0] - 2025-05-1

### Added
- Initial release (without GitHub release)

### Fixed

### Changed
