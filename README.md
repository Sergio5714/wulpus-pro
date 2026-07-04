# WULPUS PRO
## A base platform for wearable ultra-low-power ultrasound
> Independent fork by @Sergio5714 (Sergei Vostrikov)

# Introduction

This repository contains the work in progress on the WULPUS PRO ultrasound platform, a successor of the [WULPUS Project](https://github.com/Sergio5714/wulpus).

WULPUS PRO is a compact base platform for ultra-low-power ultrasound sensing.

## Comparison with original WULPUS

WULPUS PRO builds on the original [WULPUS](https://github.com/Sergio5714/wulpus) platform and keeps the same low-power wearable ultrasound philosophy, while extending the hardware and communication options.

| Feature | WULPUS | WULPUS PRO |
| --- | --- | --- |
| Number of channels | 8, time-multiplexed | **16**, time-multiplexed |
| Supported transducers | PZT transducers | PZT transducers, **CMUTs** |
| Excitation amplitude | 15 V unipolar | **30 V** unipolar |
| Excitation frequency | ~100 kHz to 4 MHz | ~100 kHz to **10 MHz** |
| Transducer biasing | - | Indirect or direct **bias**, **-30 V or 30 V** |
| Analog front-end | 10 dB LNA + 30.8 dB PGA | 6 dB LNA + **70 dB VGA** |
| TGC support | No (fixed gain) | **Yes** (linear profile) |
| Maximum PRF | 50 Hz | **300 Hz** |
| Power budget at 50 Hz PRF | <=25 mW | <=40 mW |
| Wireless link | BLE | BLE or **Wi-Fi** (via host) |
| Form factor | 46 x 25 mm footprint | **40 x 20 mm** footprint |

## System diagram

Work in progress.

## Hardware photos

Work in progress.

## Full specifications

| Feature | Specification |
| --- | --- |
| **Transducer and transmit path** | |
| Channels | 16, time-multiplexed |
| Transducer support | PZT transducers and CMUTs |
| Transducer bias | Indirect or direct bias, -30 V or 30 V |
| Excitation amplitude | 30 V unipolar |
| Excitation frequency | 100 kHz to 10 MHz |
| **Receive path** | |
| Analog front-end | 6 dB LNA + 70 dB VGA |
| Time gain compensation | TGC up to 70 dB depth-dependent attenuation compensation |
| Envelope extraction | Optional LTC5507-based envelope detector, runtime configurable |
| Envelope detector bandwidth | Up to 1.5 MHz envelope bandwidth |
| Amplification-path bandwidth | 9.9 MHz |
| End-to-end -3 dB bandwidth | 1.4 MHz (bounded by MSP430)|
| Usable measured bandwidth | SNR >=10 dB up to 2.5 MHz |
| Passband SNR | Approximately 32 dB at 40 dB total gain |
| Peak SINAD | 41.42 dB (1 MHz), 36.25 dB (2.25 MHz), both at 4.9 mV input amplitude |
| ENOB | ~7.8 bits |
| **Acquisition and data link** | |
| ADC | 8 Msps analog-to-digital converter, 12-bit resolution |
| Host interface | SPI, 8 MHz |
| Maximum PRF | 300 Hz |
| Raw streaming over BLE | 50 Hz PRF |
| Raw streaming over Wi-Fi | Up to 300 Hz PRF |
| **Imaging performance** | |
| B-mode frame rate | 18 FPS using 16-channel synthetic-aperture acquisition at 300 Hz PRF |
| B-mode axial resolution | ~ 0.7 mm with a 2.25 MHz transducer (LA-2.25-32, Vermon) |
| B-mode lateral resolution | ~ 2.3 mm at the center with a 2.25 MHz transducer (LA-2.25-32, Vermon) |
| **Power** | |
| Power budget | <=40 mW at 50 Hz PRF |
| Core electronics power | 35 mW (50 Hz PRF), 58 mW (300 Hz PRF) |
| Battery-life reference | More than 24 hours of continuous raw data streaming at 50 Hz PRF from a 300 mAh Li-Po battery with BLE |
| **Mechanics** | |
| Module size | 39 x 21 x 6 mm |
| Weight | 5 g |

# Structure of the repository

This repository has the following folders:

- `hw`, containing the CAD source files for:
  - WULPUS PRO acquisition PCB, located at `hw/wulpus_pro_acq_pcb_dev_board`
  - optional WULPUS adapter PCB for polyCMUT transducers, located at `hw/wulpus_polycmut_adapter`; this board was developed for research purposes
- `fw`, containing the firmware source code, namely:
  - MSP430 ultrasound MCU firmware required for the WULPUS PRO module, located at `fw/msp430`
  - ESP32 Wi-Fi firmware for the Seeed Studio XIAO ESP32-C6, located at `fw/esp32`
  - nRF52 firmware, located at `fw/nrf52`:
    - nRF52832 DK board firmware, located at `fw/nrf52/ble_peripheral/US_probe_nRF52_firmware`
    - nRF52840 USB dongle firmware, located at `fw/nrf52/peripheral/US_probe_dongle_firmware`
- `sw`, containing the Python code for the WULPUS PRO API and graphical user interface

# Documentation

WULPUS PRO builds on top of the original WULPUS platform. Please refer to the original [WULPUS User Manual](https://github.com/Sergio5714/wulpus/blob/main/docs/wulpus_user_manual.pdf) for assembly instructions, example measurements, and GUI overview.

For WULPUS PRO-specific documentation, see:

- [Hardware README](hw/README.md)
- [MSP430 firmware README](fw/msp430/README.md)
- [ESP32 Wi-Fi firmware README](fw/esp32/README.md)
- [nRF52 BLE firmware README](fw/nrf52/README.md)
- [Software README](sw/README.md)

# Build Instructions

To build your own instance of the WULPUS PRO platform, complete the following steps:

1. *PCB manufacturing and assembly*<br>
   Use the design files, schematics, and bills of materials under the `hw` folder. The WULPUS PRO acquisition PCB is required. The polyCMUT adapter PCB is optional and intended for research setups using polyCMUT transducers.

2. *Flash the required MSP430 firmware*<br>
   The MSP430 ultrasound MCU firmware is required for the WULPUS PRO module. Follow the instructions in `fw/msp430` to set up the toolchain, compile the firmware, and flash the MSP430 MCU.

3. *Prepare a host system*<br>
   Choose one of the supported host interfaces:

   - ESP32-based Wi-Fi host, for example the Seeed Studio XIAO ESP32-C6. Follow the instructions in [fw/esp32/README.md](fw/esp32/README.md) to build and flash the ESP32 firmware. After flashing, complete the provisioning step so WULPUS PRO can connect to your Wi-Fi network.
   - nRF52 BLE host, using the nRF52832 DK board and nRF52840 USB dongle. This is the legacy BLE setup. Follow the instructions in [fw/nrf52/README.md](fw/nrf52/README.md) to compile and flash both firmwares.

4. *Python dependencies installation on the host PC*<br>
   Follow the instructions in `sw` to install the Python dependencies. With `uv`, the basic setup is:

   ```bash
   uv sync
   uv run jupyter notebook
   ```

# Usage

## Wi-Fi setup with XIAO ESP32-C6

1. Connect the ESP32 host module to the WULPUS PRO board using the pin mapping documented in [fw/esp32/README.md](fw/esp32/README.md).
2. Power the ESP32-based host module, for example the Seeed Studio XIAO ESP32-C6 running the firmware from `fw/esp32`.
3. Power the WULPUS PRO board through the connector or debug pin headers.
4. Start Jupyter from the `sw` folder:

   ```bash
   uv run jupyter notebook
   ```

5. Open `wulpus_pro_wifi_example.ipynb` in the browser and follow the notebook instructions for discovery, connection setup, configuration transfer, and acquisition.

## BLE setup with nRF52 (legacy)

1. Connect the nRF52 DK to the WULPUS PRO board using the pin mapping documented in [fw/nrf52/README.md](fw/nrf52/README.md).
2. Plug in the USB dongle and power the nRF52 DK via USB.
3. Check the dongle connection. The green LED should light up. If it does not, press the reset button on the nRF52 DK and try again.
4. After confirming dongle connectivity, power the WULPUS PRO board through the connector or debug pin headers.
5. Start Jupyter from the `sw` folder:

   ```bash
   uv run jupyter notebook
   ```

6. Open `wulpus_pro_gui.ipynb` in the browser and follow the notebook instructions to begin acquisition.

# Citation

If you would like to cite this repository, please use:

```bibtex
@misc{wulpus_pro_repo_sergio5714_2026,
  title={WULPUS PRO: A Base Platform for Wearable Ultra-Low-Power Ultrasound (Independent Fork)},
  author={Vostrikov, Sergei and Villani, Federico and Hirschi, Cedric and Cossettini, Andrea and Benini, Luca},
  year={2026},
  howpublished={GitHub repository},
  url={https://github.com/Sergio5714/wulpus-pro}
}
```

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release notes and main project changes.

# Authors

This fork is maintained independently by @Sergio5714 ([Sergei Vostrikov](https://scholar.google.com/citations?user=a0KNUooAAAAJ&hl=en)).

The WULPUS PRO system was originally developed at the [Integrated Systems Laboratory (IIS)](https://iis.ee.ethz.ch/) at ETH Zurich by:

- [Sergei Vostrikov](https://scholar.google.com/citations?user=a0KNUooAAAAJ&hl=en) (PCB design, firmware, software, open-sourcing)
- [Federico Villani](https://scholar.google.com/citations?user=5LgLMCEAAAAJ&hl=en) (PCB design, component selection)
- [Cedric Hirschi](https://www.linkedin.com/in/c%C3%A9dric-cyril-hirschi-09624021b/) (firmware, software)
- [Andrea Cossettini](https://scholar.google.com/citations?user=d8O91jIAAAAJ&hl=en) (supervision, project administration)
- [Luca Benini](https://scholar.google.com/citations?hl=en&user=8riq3sYAAAAJ) (supervision, project administration)

Thanks to all the people who contributed to the WULPUS PRO platform:

- [Sebastian Frey](https://scholar.google.com/citations?user=7jhiqz4AAAAJ&hl=en) (design review)

# License

The following files are released under Apache License 2.0 (`Apache-2.0`) (see `sw/LICENSE`):

- `sw/`

The following files are released under Solderpad v0.51 (`SHL-0.51`) (see `hw/LICENSE`):

- `hw/`

The `fw/msp430/`, `fw/nrf52/`, and `fw/esp32/` directories contain third-party sources that come with their own licenses. See the respective folders and source files for the licenses used.

## Limitation of Liability

In no event and under no legal theory, whether in tort (including negligence), contract, or otherwise, unless required by applicable law (such as deliberate and grossly negligent acts) or agreed to in writing, shall any Contributor be liable to You for damages, including any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or out of the use or inability to use the Work (including but not limited to damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses), even if such Contributor has been advised of the possibility of such damages.
