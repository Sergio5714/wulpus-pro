# Changelog

All main changes to this project will be documented in this file.
For the detailed description, please explore nested folders and corresponding CHANGELOG.md files (e.g. for PCB projects or firmware). 

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-07-11

### Added

- Added a linked table of contents to the root README.
- Added the WULPUS PRO arXiv preprint citation and BibTeX entry to the root README.
- Added PCBWay shared-project production and assembly information to the hardware README.
- Expanded the root README acknowledgements.

## [1.0.0] - 2026-07-11

### Added

- Added ESP32-C6 Wi-Fi firmware support for the Seeed Studio XIAO ESP32-C6 board, including board-specific defaults and README pin mapping.
- Added MSP430 board reset control logic to the ESP32 Wi-Fi firmware.
- Added a Python Wi-Fi example notebook for discovery, connection setup, configuration transfer, and data acquisition over the ESP32 TCP link.
- Added Apache-2.0 license headers to ESP32 firmware source files and Python support scripts.
- Added a documentation changelog under `docs/`.
- Added image documentation README with CC BY-ND 4.0 license information.
- Added WULPUS PRO images to the root README hero and hardware photos sections.
- Added the WULPUS PRO system diagram to the root README.
- Added `uv` project metadata and lockfile for Python dependency management.
- Added a root README specifications section comparing WULPUS PRO with the original WULPUS platform.

### Fixed

- Fixed the MSP430 firmware initialization so the preamplifier power switch is always enabled at startup.
- Fixed the ESP32-C6-DEVKITM-1 board defaults to avoid reusing GPIO2 for both SPI MOSI and MSP430 reset.

### Changed

- Refactored the ESP32 firmware configuration around reusable ESP-IDF defaults, target-specific defaults, and board-specific pinout files.
- Updated the ESP32 project from ESP-IDF 5.4.1 to ESP-IDF 6.0.1.
- Updated ESP32 documentation for tested DevKit and XIAO boards, board selection, flashing, serial port selection, and WULPUS PRO connector pin mappings.
- Updated ESP32 firmware attribution with author information and ESP-IDF example note.
- Reworked the root README introduction with a concise overview of the pulser, acquisition front end, supported transducers, and module form factor.
- Updated project READMEs for repository structure, build instructions, usage flows, firmware links, and WULPUS PRO-specific documentation pointers.
- Migrated Python package management from conda to uv.
- Moved WULPUS PRO full specifications from the root README into `docs/full_specifications.md`.
- Updated gitignore rules.

### Removed

- Removed the old ESP32-C6-DEVKITM-1 MSP430 reset mapping from GPIO2; GPIO2 is used for SPI MOSI and MSP430 reset is now on GPIO3.
- Removed the old Conda `sw/requirements.yml` dependency file.

## [0.1.0] - 2025-05-1

### Added
- Initial release (without official GitHub release)

### Fixed

### Changed
