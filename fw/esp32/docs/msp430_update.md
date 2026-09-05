# MSP430 firmware updates through the ESP32

The ESP32 can stage an MSP430FR5043 firmware image received over USB CDC or
Wi-Fi/TCP, then program the MSP430 through four-wire JTAG. This updates the
MSP430 application; it does not update the ESP32 firmware.

The JTAG programmer is based on TI's
[MSP430 Programming With the JTAG Interface (SLAU320AJ)](https://www.ti.com/lit/pdf/slau320)
and its MSP430 FRAM Replicator reference implementation:
[JTAGfunc430FR.c](../components/msp430_programmer/ti/JTAGfunc430FR.c) and
[JTAGfunc430FR.h](../components/msp430_programmer/ti/JTAGfunc430FR.h).
[msp430_jtag.c](../components/msp430_programmer/msp430_jtag.c) and the adapted
[LowLevelFunc430Xv2.h](../components/msp430_programmer/ti/LowLevelFunc430Xv2.h)
provide the ESP32 GPIO interface.

## Requirements and wiring

Install ESP32 firmware built with the current `partitions.csv`, including the
256 KiB `msp_image` data partition (subtype `0x40`, following the 2 MiB factory
application at `0x210000`). When upgrading an older ESP32 installation, flash
the partition table as well as the application; an application-only replacement
does not create this partition. Programmer initialization requires it.

The XIAO defaults assign these additional signals:

| MSP430 signal | ESP32-C6 GPIO | XIAO pin |
|---|---:|---|
| RST/NMI | 0 | D0 |
| TEST | 2 | D2 |
| TDO | 21 | D3 |
| TDI/TCLK | 22 | D4 |
| TMS | 23 | D5 |
| TCK | 16 | D6 |

Connect all six signals and a common ground, with compatible logic supplies.
The SPI and DATA_READY connections alone are insufficient. These are firmware
assignments: verify the actual PCB or bench wiring. Other board configurations
must explicitly check `CONFIG_WP_GPIO_MSP_*` against their available GPIOs.
In particular, the ESP32-C6-DEVKITM-1 acquisition defaults inherit
`CONFIG_WP_GPIO_MSP_TEST=2` from the common C6 defaults, while SPI MOSI also
uses GPIO2. Select a non-conflicting TEST pin and verify every JTAG signal
before using the updater on that board; the XIAO mapping cannot be used
unchanged on the DevKit.
Disconnect an external MSP-FET before letting the ESP32 drive JTAG. The ESP32
releases TEST and the four JTAG data/clock pins to inputs after programming;
MSP430 reset remains under the board component's control.

## Updating from Jupyter

1. Build the MSP430 firmware in CCS using **Debug**, which currently enables
   TI-TXT export. See the [MSP430 build instructions](../../msp430/README.md).
2. Stop acquisition and close the acquisition GUI or any serial monitor.
3. Open [`sw/msp430_update.ipynb`](../../../sw/msp430_update.ipynb), run the USB
   connection cell, select the COM port, and click **Open**.
4. Select `.txt`/`.titxt` (TI-TXT), `.hex`/`.ihex` (Intel HEX), or a packaged
   `.mspfw` file, then click **Upload and program**.
5. Expect an ESP32 reboot and a temporary loss of connectivity after upload.
   Programming runs before USB/TCP application listeners start. Use **Check
   status** after reconnection if automatic recovery times out.
6. Require `COMPLETE` with error zero, then reconnect the acquisition client,
   send its acquisition configuration, and check normal operation.

The notebook connection widget is USB-specific. The `MSP430Updater` API also
accepts an already-open `WulpusProWiFiLink`:

```python
from wulpus.msp430_update import MSP430Updater, MSP430UpdateState

# link is an already-open, idle USB CDC or Wi-Fi link.
updater = MSP430Updater(link)
result = updater.program_file("firmware.txt", timeout=120)
print(result)
if result.state != MSP430UpdateState.COMPLETE or result.error:
    print(updater.diagnostics())
    raise RuntimeError("MSP430 firmware update did not complete")
```

The Python uploader uses 128-byte chunks by default, checks offset/sequence
acknowledgements, and retries explicitly CRC-rejected chunks up to three times.
It does not blindly retry chunks after an ambiguous transport failure. There
is no supported resumable upload: abort an incomplete upload before beginning
again. The timeout passed to `program_file` governs the post-commit wait.

## Commit, completion, and recovery

Upload stages the complete image in ESP32 flash. `COMMIT` persists a pending
update in NVS and schedules an ESP32 reboot after approximately 250 ms. Its
acknowledgement means that the update was scheduled, not that programming passed.
On reboot, the ESP32 validates image CRCs, ranges, and target, writes the listed
FRAM sections, verifies them using TI's verification routine, resets/releases
the target, and persists the outcome and JTAG diagnostics. Normal startup then
continues. Unlisted target memory is not erased by this section-writing path.

There is no live protocol progress while the boot-time programmer runs.
`COMPLETE` means programming and verification succeeded; it does not establish
that the new application booted correctly or produced valid acquisitions.
`RESETTING` and `WAITING_FOR_BOOT` are defined status values but are not emitted
by the current boot-time path.

An interrupted programming attempt is marked failed at the next startup rather
than automatically resumed. There is no dual-image rollback on the MSP430. A
failed or interrupted write can require another complete update or recovery
with an external programmer. `ABORT` is useful before commit; it does not undo
a committed boot-time update or reliably cancel its scheduled reboot.

The updater checks the target descriptor against `0x8317` (FR5043); the image
container's target identifier is separately `0x00005043`. Diagnostics report
JTAG/core IDs, the descriptor pointer, control signals, and quick/direct device
reads. A failed identification with zero processed bytes indicates the write
phase was not reached. CRC checks detect corruption; images are not signed.

## Image container

All integers are little-endian. Text images are converted to an `MSP1` container
before upload; `.out` ELF and raw flat `.bin` files are not accepted directly.

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
CRC-32 polynomial `0xEDB88320`, compatible with Python `zlib.crc32`.

Sections must be nonoverlapping and word-aligned, lie in
`[0x6000, 0x15FF8)`, and exclude `[0xFF80, 0xFF90)` (JTAG/BSL signatures).
Interrupt vectors outside that protected range are allowed. The text importer
removes erased `0xFF` signature placeholders, rejects other signature contents,
rejects odd start addresses, and pads odd data lengths with `0xFF`.

See the [PC protocol](esp_protocol.md#msp430-update-protocol) for upload commands
and status layouts.
