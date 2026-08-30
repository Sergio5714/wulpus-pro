# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Selectable Wi-Fi and BLE communication transports in `WulpusGuiSingleCh`.
- Native ESP32-C6 USB CDC transport for wired WULPUS PRO communication.
- Added a standalone USB frame-rate profiler with acquisition-number gap,
  timeout, latency, and achieved-FPS reporting.
### Fixed

- Worked around the Windows pySerial `SetCommState` error 31 affecting the
  ESP32-C6 USB Serial/JTAG CDC port while preserving all other configuration
  errors.
- Removed multi-second USB receive stalls by blocking for the first byte and
  then draining only the bytes already available from the serial driver.
- Prevented asynchronous `GET_DATA` frames from being mistaken for command
  acknowledgements, including during acquisition shutdown.
- Preserved partial serial writes and reported write timeouts instead of
  silently truncating protocol packets.

### Changed

- Renamed the BLE serial transport from `dongle.py` / `WulpusDongle` to `ble_dongle.py` / `WulpusBleDongle`.
- Decoupled `WulpusGuiSingleCh` from the BLE transport and made its device controls transport-neutral.
- Renamed the Wi-Fi transport and discovery APIs for WULPUS PRO and hardened TCP framing, command handling, connection cleanup, and acquisition lifecycle management.
- Reorganized `wulpus_pro_wifi_example.ipynb` to launch the GUI after explicit TX/RX and interactive ultrasound configuration, followed by the detailed manual Wi-Fi workflow.
- Added single-owner TCP/USB session arbitration to the ESP32 firmware while preserving the existing WULPUS wire protocol.
- Made the common framed-packet backlog and parser serve both command responses
  and acquisition data so packet boundaries remain synchronized.

### Removed

- Removed the legacy 8-channel WULPUS configuration, packet, and channel-GUI modules. This repository now exposes only the WULPUS PRO configuration stack.

## [0.1.0] - 2025-05-01

### Added

- GUI and library from WULPUS repository version 1.2.2
- A curve to the main GUI, visualizing the gain profile over time.

### Fixed

### Changed

- Extended the number of channels to 16.
- Modified TX/RX pin mapping.
- Added two new configuration parameters for VGA control:
    - `VGA Precharge time [cycles]`
    - `Wiper code for gain slope []`

- Extended the configuration package to accomodate two new parameters.

### Removed
- Removed `Capture restart time` and `Capture timeout time` from the old GUI.
