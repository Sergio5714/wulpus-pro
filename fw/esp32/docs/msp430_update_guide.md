# MSP430 firmware update guide

The ESP32 can stage an MSP430FR5043 firmware image received over USB CDC, then
program the MSP430 through four-wire JTAG. This updates the MSP430 application;
it does not update the ESP32 firmware.

## Contents

- [Requirements](#requirements)
- [Update the MSP430 over USB](#update-the-msp430-over-usb)
- [Completion and recovery](#completion-and-recovery)
- [Technical reference: image container](#technical-reference-image-container)

The JTAG programmer is based on TI's
[MSP430 Programming With the JTAG Interface (SLAU320AJ)](https://www.ti.com/lit/pdf/slau320)
and its MSP430 FRAM Replicator reference implementation:
[JTAGfunc430FR.c](../components/msp430_programmer/ti/JTAGfunc430FR.c) and
[JTAGfunc430FR.h](../components/msp430_programmer/ti/JTAGfunc430FR.h).
[msp430_jtag.c](../components/msp430_programmer/msp430_jtag.c) and the adapted
[LowLevelFunc430Xv2.h](../components/msp430_programmer/ti/LowLevelFunc430Xv2.h)
provide the ESP32 GPIO interface.

## Requirements

- A WULPUS PRO WiFi host PCB connected to the Acquisition PCB.
- A data-capable USB-C cable.
- ESP32 firmware built with the current `partitions.csv` and flashed as a
  complete image, including the partition table.
- An MSP430 TI-TXT image built with the CCS **Debug** configuration. See the
  [MSP430 build instructions](../../msp430/README.md).
- The Python dependencies installed as described in the
  [software setup](../../../sw/README.md#how-to-get-started).

The WiFi host PCB contains all required MSP430 JTAG connections; no external
programmer or wiring is required. Disconnect any attached MSP-FET before using
the updater.

> A standalone XIAO ESP32-C6 cannot use this updater because the required JTAG
> signals are not exposed on the Acquisition PCB's Dupont connectors.

## Update the MSP430 over USB

1. Stop acquisition and close the acquisition GUI and any serial monitor.
2. Connect the WiFi host PCB to the PC with a data-capable USB-C cable and keep
   the system powered throughout the update.
3. Open [`sw/msp430_update.ipynb`](../../../sw/msp430_update.ipynb), run the USB
   connection cell, select the ESP32-C6 COM port, and click **Open**.
4. Select the TI-TXT image and click **Upload and program**.
5. Wait while the ESP32 reboots and programs the MSP430. Do not disconnect the
   USB cable or remove power.
6. Reconnect and click **Check status** if the result is not restored
   automatically. Require `COMPLETE` with error zero.
7. Reopen the acquisition notebook, apply its configuration, and verify normal
   acquisition.

## Completion and recovery

The ESP32 temporarily disconnects from USB while it programs and verifies the
MSP430 during startup. `COMPLETE` confirms that programming and verification
succeeded; verify acquisition separately to confirm that the application runs
correctly.

An interrupted update is not resumed automatically, and the MSP430 has no
dual-image rollback. Retry the complete update after reconnecting. If the
updater cannot identify or program the MSP430, recovery may require an external
programmer.

## Technical reference: image container

All integers are little-endian. The TI-TXT image is converted to an `MSP1`
container before upload; `.out` ELF and raw flat `.bin` files are not accepted
directly.

| Header field | Bytes | Value |
|---|---:|---|
| magic | 4 | `0x3150534D` (`MSP1`) |
| version, header_size | 2 each | `1`, `24` |
| target_id | 4 | `0x00005043` |
| total_size | 4 | Header + section table + data |
| image_crc32 | 4 | CRC of concatenated section data |
| section_count, flags | 2 each | 1–64 sections; flags zero |

The header is followed by one 12-byte `(address, length, data_crc32)` descriptor
per section, then the section data in table order. CRCs use the reflected
CRC-32 polynomial `0xEDB88320`.

Sections must be nonoverlapping and word-aligned, lie in
`[0x6000, 0x15FF8)`, and exclude `[0xFF80, 0xFF90)` (JTAG/BSL signatures).
Interrupt vectors outside that protected range are allowed. The text importer
removes erased `0xFF` signature placeholders, rejects other signature contents,
rejects odd start addresses, and pads odd data lengths with `0xFF`.

See the [MSP430 firmware update protocol](msp430_update_protocol.md) for upload
commands and status layouts.
