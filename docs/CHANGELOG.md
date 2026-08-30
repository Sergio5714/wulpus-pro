# Changelog

All notable changes to the WULPUS PRO documentation will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added WULPUS PRO full specifications as a dedicated documentation page.
- Added project-level documentation changelog.
- Added image documentation README with authorship and CC BY-ND 4.0 license information.
- Added WULPUS PRO images to the root README hero and hardware photos sections.
- Added the WULPUS PRO system diagram to the root README.
- Documented native ESP32-C6 USB CDC operation, TCP/USB session arbitration,
  flashing and JTAG coexistence, binary-protocol logging constraints, and USB
  frame-rate profiling.

### Changed

- Moved the full specifications table from the root README to `docs/full_specifications.md`.
- Updated ESP32 and Python usage documentation for transport selection, USB
  connection ownership, and power-management behavior while a USB host is
  connected.
- Documented the task-based SPI DMA and packet-transmission architecture and
  runtime status commands.
