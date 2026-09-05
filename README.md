# WULPUS PRO
## A base platform for wearable ultra-low-power ultrasound
> Independent fork by @Sergio5714 (Sergei Vostrikov)

<p align="center">
  <img src="docs/images/v1_0/wulpus_pro_main.png" alt="WULPUS PRO main" width="80%"/>
  <br/>
  WULPUS PRO module with PolyCMUT transducer.
</p>

## Table of contents

- [Introduction](#introduction)
  - [System diagram](#system-diagram)
  - [Hardware photos](#hardware-photos)
  - [Specifications](#specifications)
- [Clone the repository](#clone-the-repository)
- [Structure of the repository](#structure-of-the-repository)
- [Documentation](#documentation)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
  - [WiFi or USB setup: WULPUS PRO WiFi host PCB](#wifi-or-usb-setup-wulpus-pro-wifi-host-pcb)
  - [BLE setup: WULPUS PRO + nRF52 DK (legacy)](#ble-setup-wulpus-pro--nrf52-dk-legacy)
- [Citation](#citation)
- [Changelog](#changelog)
- [Authors](#authors)
- [License](#license)
  - [Limitation of Liability](#limitation-of-liability)

# Introduction

This repository contains work in progress on the WULPUS PRO ultrasound platform, a successor to the [WULPUS Project](https://github.com/Sergio5714/wulpus). It features a programmable 30 V unipolar pulser, a time-multiplexed multichannel acquisition front end with TGC, an optional envelope extractor, and support for PZT and CMUT transducers. The module is compact (40 x 20 mm footprint) and lightweight (5 g), allowing integration with an external host PCB.

## System diagram

<p align="center">
  <img src="docs/images/wulpus_pro_system_diagram.png" alt="WULPUS PRO system diagram" width="100%"/>
  <br/>
  WULPUS PRO system diagram
</p>

## Hardware photos

<table>
  <tr>
    <td colspan="2" align="center">
      <img src="docs/images/v1_0/eval_board.jpg" alt="WULPUS PRO evaluation board" width="80%"/>
      <br/>
      WULPUS PRO evaluation board
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <img src="docs/images/v1_0/eval_board_top.jpg" alt="Top view of the WULPUS PRO evaluation board" width="100%"/>
      <br/>
      Top view
    </td>
    <td width="50%" align="center">
      <img src="docs/images/v1_0/eval_board_bottom.jpg" alt="Bottom view of the WULPUS PRO evaluation board" width="100%"/>
      <br/>
      Bottom view
    </td>
  </tr>
</table>

## Specifications

WULPUS PRO builds on the original [WULPUS](https://github.com/Sergio5714/wulpus) platform and keeps the same low-power wearable ultrasound philosophy while extending the hardware and communication options. The table below compares the main features of WULPUS PRO with those of the original WULPUS platform.

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
| Wireless link | BLE | BLE or **WiFi** (via host) |
| Form factor | 46 x 25 mm footprint | **40 x 20 mm** footprint |

Full WULPUS PRO specifications are available in [docs/full_specifications.md](docs/full_specifications.md).

# Clone the repository

Clone the public repository without its private development submodule:

```bash
git clone https://github.com/Sergio5714/wulpus-pro.git
```

The `hw/kicad_us_lib` submodule is an internal development library hosted in a
private repository. External users do not have access to it and do not need to
initialize it to use the released fabrication outputs. For editing the KiCad
designs, generate project-specific symbol and footprint libraries from the
symbols and footprints embedded in the corresponding project files. See the
[hardware README](hw/README.md#internal-kicad-library) for details.

Authorized developers can initialize the private submodule after cloning:

```bash
git submodule update --init --recursive
```

# Structure of the repository

The repository is organized into four top-level folders. See the README files
inside them for component-level details.

- [`hw`](hw) — PCB design and fabrication files.
- [`fw`](fw) — embedded firmware source code.
- [`sw`](sw) — Python software, graphical interfaces, and notebooks.
- [`docs`](docs) — project documentation and images.

# Documentation

- [Full system specifications](docs/full_specifications.md)
- [PCB designs and fabrication](hw/README.md)
- Firmware:
  - [ESP32 host firmware](fw/esp32/README.md)
  - [MSP430 acquisition firmware](fw/msp430/README.md)
  - [Legacy nRF52 BLE firmware](fw/nrf52/README.md)
- [Python software and notebooks](sw/README.md)

For background information about the original WULPUS platform, see the
[legacy WULPUS User Manual](https://github.com/Sergio5714/wulpus/blob/main/docs/wulpus_user_manual.pdf).

# Build Instructions

The WULPUS PRO WiFi host PCB provides a one-cable setup and programming
workflow:

1. **Get hardware**

   Order the [Acquisition PCB](hw/wulpus_pro_acq_pcb_dev_board) and [WiFi host PCB](hw/wulpus_wifi_host_pcb) using the [PCBWay shared projects](hw/README.md#pcbway-shared-projects), or manufacture and assemble them yourself using the design files, schematics, and bills of materials linked in the [hardware guide](hw/README.md#included-pcb-designs).

2. **Install the host software**

   Follow the [Python software setup instructions](sw/README.md#how-to-get-started) on the host PC.

3. **Compile the firmware images**

   Build the two required firmware images using the dedicated instructions:

   - [Install the ESP-IDF toolchain and compile the ESP32 firmware](fw/esp32/docs/development.md).
   - [Install the MSP430 toolchain and compile the MSP430 firmware](fw/msp430/README.md#build-and-export-firmware).

4. **Connect one USB cable and flash both controllers**

   - Connect the WiFi host PCB to the Acquisition PCB, then connect the host PCB to the PC with a single USB-C cable.
   - First, [flash the ESP32 firmware](fw/esp32/docs/development.md#flash-the-firmware), then reset the board.
   - Then use the same USB connection and the [MSP430 updater GUI](fw/esp32/docs/msp430_update.md#updating-from-jupyter) to program the MSP430 on the Acquisition PCB.

   > No external programmer, adapter board, or additional programming cables are required for this workflow.

# Usage

## WiFi or USB setup: WULPUS PRO WiFi host PCB

1. Connect the [WULPUS PRO WiFi host PCB](hw/wulpus_wifi_host_pcb) to the [Acquisition PCB](hw/wulpus_pro_acq_pcb_dev_board).
2. Connect the host PCB to the PC with a data-capable USB-C cable, then reset the board. This connection powers both PCBs.
3. Start Jupyter from the `sw` folder:

   ```bash
   uv run jupyter notebook
   ```

4. Open [`wulpus_pro_example.ipynb`](sw/wulpus_pro_example.ipynb) in the browser.
5. For a wired connection, select **USB CDC**, scan for devices, and open the ESP32-C6 port. For a wireless connection, first complete [WiFi provisioning](fw/esp32/docs/provisioning.md), then select **WiFi** and discover the device.
6. Apply the acquisition configuration in the notebook and start acquisition.

For development with a standalone XIAO ESP32-C6, follow its [wiring and power requirements](fw/esp32/docs/development.md#supported-boards) and the [pin mapping](fw/esp32/docs/development.md#pin-mapping). This setup requires an Acquisition PCB with the MSP430 firmware already programmed.

## BLE setup: WULPUS PRO + nRF52 DK (legacy)

1. Connect the nRF52 DK to the Acquisition PCB using the pin mapping documented in [fw/nrf52/README.md](fw/nrf52/README.md).
2. Plug in the USB dongle and power the nRF52 DK via USB.
3. Check the dongle connection. The green LED should light up. If it does not, press the reset button on the nRF52 DK and try again.
4. After confirming dongle connectivity, power the Acquisition PCB through the connector or debug pin headers.
5. Start Jupyter from the `sw` folder:

   ```bash
   uv run jupyter notebook
   ```

6. Open [`wulpus_pro_example.ipynb`](sw/wulpus_pro_example.ipynb) in the browser, select **BLE**, and follow the notebook instructions to begin acquisition. Older notebook variants are archived under `sw/legacy`.

# Citation

Please cite our [arXiv preprint](https://arxiv.org/abs/2607.12137):

```bibtex
@article{vostrikov2026wulpuspro,
  title={WULPUS PRO: Multi-mode Ultra-Low-Power Wearable Ultrasound and Array Imaging with CMUT Support},
  author={Vostrikov, Sergei and Villani, Federico and Hirschi, Cedric and Lu, Jinhao and Welsch, Jonas and Angerer, Martin and Cretu, Edmond and Rohling, Robert and Cossettini, Andrea and Benini, Luca},
  journal={arXiv preprint arXiv:2607.12137},
  year={2026},
  url={https://arxiv.org/abs/2607.12137}
}
```

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

Since the conclusion of the original ETH Zurich project in 2025, this
repository and the continued development of WULPUS PRO have been maintained
independently by [Sergei Vostrikov](https://scholar.google.com/citations?user=a0KNUooAAAAJ&hl=en)
(@Sergio5714), with contributions from others.

The initial WULPUS PRO system was developed as a research project at the
[Integrated Systems Laboratory (IIS)](https://iis.ee.ethz.ch/) at ETH Zurich
from 2024 to 2025 by:

- [Sergei Vostrikov](https://scholar.google.com/citations?user=a0KNUooAAAAJ&hl=en) (PCB design, firmware, software, open-sourcing)
- [Federico Villani](https://scholar.google.com/citations?user=5LgLMCEAAAAJ&hl=en) (PCB design, component selection)
- [Cedric Hirschi](https://www.linkedin.com/in/c%C3%A9dric-cyril-hirschi-09624021b/) (firmware, software)
- [Andrea Cossettini](https://scholar.google.com/citations?user=d8O91jIAAAAJ&hl=en) (supervision, project administration)
- [Luca Benini](https://scholar.google.com/citations?hl=en&user=8riq3sYAAAAJ) (supervision, project administration)

Additional contributors to the project were:

- [Sebastian Frey](https://scholar.google.com/citations?user=7jhiqz4AAAAJ&hl=en), ETH Zürich (design review)
- [Alfonso Blanco Fontao](https://www.linkedin.com/in/alfonso-blanco-fontao-b6214726/), ETH Zürich (design review, PCB fabrication coordination)
- [Ciara Giles Doran](https://www.linkedin.com/in/ciaragilesdoran/) (preliminary evaluation of the AD8338 VGA)

# License

The following files are released under Apache License 2.0 (`Apache-2.0`) (see `sw/LICENSE`):

- `sw/`

The hardware designs are released under Solderpad v0.51 (`SHL-0.51`):

- `hw/`

See the [hardware license table](hw/README.md#license) for the applicable license file and copyright holder for each PCB design.

Project-authored firmware code is generally licensed under Apache-2.0, as
identified in the source-file headers. Bundled vendor and third-party code
retains its original license, including BSD-style terms for Texas Instruments
sources and the Nordic Semiconductor license for Nordic SDK sources. See the
license notices and source headers in `fw/esp32/`, `fw/msp430/`, and `fw/nrf52/`
for the terms that apply to each file.

## Limitation of Liability

In no event and under no legal theory, whether in tort (including negligence), contract, or otherwise, unless required by applicable law (such as deliberate and grossly negligent acts) or agreed to in writing, shall any Contributor be liable to You for damages, including any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or out of the use or inability to use the Work (including but not limited to damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses), even if such Contributor has been advised of the possibility of such damages.
