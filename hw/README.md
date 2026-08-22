# WULPUS PRO PCB design files

This directory contains the PCB designs used by WULPUS PRO. The source files are
provided in Altium Designer or KiCad format, depending on the board. Where
available, each project also includes PDF schematics and assembly drawings under
`docs`, and production-ready files under `fabrication_outputs`, including bills
of materials, Gerber files, NC drill files, and pick-and-place files.

## Included PCB designs

### WULPUS PRO acquisition board

Directory: [`wulpus_pro_acq_pcb_dev_board`](wulpus_pro_acq_pcb_dev_board)

The main WULPUS PRO ultrasound acquisition and pulser board. This board contains
the transmit and receive signal paths, high-voltage switching, power supplies,
MSP430 control circuitry, and the host interface. The design sources are provided
in Altium Designer format. This board is required to build a WULPUS PRO system.

### WULPUS PRO Wifi host

Directory: [`wulpus_wifi_host_pcb`](wulpus_wifi_host_pcb)

A dedicated wireless host board for WULPUS PRO, based on the Seeed Studio XIAO
ESP32-C6. It connects to the acquisition board and runs the ESP32 firmware in
[`../fw/esp32`](../fw/esp32), providing Wi-Fi control and data transfer. The
design sources are provided in KiCad format, together with fabrication and
assembly outputs.

### polyCMUT adapter board

Directory: [`wulpus_polycmut_adapter`](wulpus_polycmut_adapter)

An optional research adapter for connecting polyCMUT transducers to WULPUS. The
design sources are provided in Altium Designer format. This board is not required
for standard WULPUS PRO operation.

## Internal KiCad library

The [`kicad_us_lib`](kicad_us_lib) directory is referenced as a Git submodule and
contains the internal KiCad symbols, footprints, and related library assets used
to develop the hardware designs. It is hosted in a private repository and is
intended only for internal development; external users will not have access to
it. The library could not be made publicly open source because some of its
contents are subject to third-party licensing restrictions.

The released KiCad projects embed the symbols and footprints used by each
design. External users are advised to generate their own project-specific symbol
and footprint libraries from the corresponding project files instead of relying
on `kicad_us_lib`. The released fabrication outputs can be used without the
private submodule.

## PCBWay shared projects

For convenient one-click PCB production and assembly, you can use the PCBWay shared projects (optional; DIY using the design and fabrication files in this folder is also supported):

- [WULPUS PRO Evaluation Board v1.0.0](https://www.pcbway.com/project/shareproject/WULPUS_PRO_Evaluation_board_v1_0_0_992d7510.html)

This option is convenient for outsourced PCB production and assembly, with an estimated **~1 month lead time** and a price of about **USD 220** per probe (evaluation board), based on mid-2026 pricing.

## License

All three PCB designs are released under the Solderpad Hardware License v0.51
(`SHL-0.51`). They use separate license files because their copyright holders
differ:

| PCB design | Directory | License file | Copyright |
| --- | --- | --- | --- |
| WULPUS PRO acquisition board | [`wulpus_pro_acq_pcb_dev_board`](wulpus_pro_acq_pcb_dev_board) | [`LICENSE_ETH`](LICENSE_ETH) | Copyright (C) 2025 ETH Zurich. All rights reserved. |
| WULPUS polyCMUT adapter | [`wulpus_polycmut_adapter`](wulpus_polycmut_adapter) | [`LICENSE_ETH`](LICENSE_ETH) | Copyright (C) 2025 ETH Zurich. All rights reserved. |
| WULPUS PRO Wifi host | [`wulpus_wifi_host_pcb`](wulpus_wifi_host_pcb) | [`LICENSE`](LICENSE) | Copyright (C) 2026 Sergei Vostrikov. All rights reserved. |

The license terms are otherwise identical. The internal `kicad_us_lib`
submodule contains third-party material and is not covered by this table; its
contents retain their respective license terms.
