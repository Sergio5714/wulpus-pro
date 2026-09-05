# WULPUS PRO source files for MSP430 Ultrasound MCU firmware project
This directory contains the source firmware files for 
- MSP430FR5043 Ultrasound MCU (`fw/msp430/wulpus_msp430_firmware`) mounted on the WULPUS PRO PCB

# Build and export firmware

Import `wulpus_msp430_firmware` into Code Composer Studio as an existing CCS
project. Its CCS project name is `wulpus_pro_msp430_firmware` and it uses the TI
MSP430 21.6 compiler family. Build **Debug** to produce the `.out` executable
and a TI-TXT image: the Debug configuration now enables the MSP430 Hex Utility
with TI-TXT output and 8-bit memory/ROM widths. Release does not currently
enable that export step. The project preferences allow flexible compiler/product
version matching, so pin the installed toolchain for reproducible builds.

## Command-line build (CCS 11 on Windows)

From the repository root, use a dedicated workspace for automation. Adjust
the CCS installation path for your machine. Import once per workspace, then
run the build command for subsequent builds. Do not concurrently build the same
project from the IDE and automation.

```powershell
$ccs = "C:\ti\ccs1100\ccs\eclipse\eclipsec.exe"
$project = Join-Path $PWD "fw\msp430\wulpus_msp430_firmware"
$workspace = Join-Path $env:TEMP "wulpus-ccs-build"

& $ccs -noSplash -data $workspace `
    -application com.ti.ccstudio.apps.projectImport -ccs.location $project
if ($LASTEXITCODE -ne 0) { throw "CCS import failed" }

& $ccs -noSplash -data $workspace `
    -application com.ti.ccstudio.apps.projectBuild `
    -ccs.projects wulpus_pro_msp430_firmware `
    -ccs.configuration Debug -ccs.buildType full -ccs.listErrors
if ($LASTEXITCODE -ne 0) { throw "CCS build failed" }
```

To explicitly convert the executable to a predictably named TI-TXT file:

```powershell
$hex = "C:\ti\ccs1100\ccs\tools\compiler\ti-cgt-msp430_21.6.0.LTS\bin\hex430.exe"
& $hex --ti_txt --memwidth=8 --romwidth=8 `
    --outfile="$project\Debug\firmware.txt" `
    "$project\Debug\wulpus_pro_msp430_firmware.out"
if ($LASTEXITCODE -ne 0) { throw "TI-TXT export failed" }
```

These commands target Eclipse-based CCS 11; newer CCS generations use a
different launcher. See TI's [CCS command-line build documentation](https://software-dl.ti.com/ccs/esd/documents/ccs_projects-command-line.html).

# Flashing

For the ESP32 host, use [`sw/msp430_update.ipynb`](../../sw/msp430_update.ipynb)
to upload a TI-TXT, Intel HEX, or `.mspfw` image over USB. The Python updater
also supports Wi-Fi. The ESP32 stages the image, reboots, programs and verifies
the MSP430 through four-wire JTAG, then exposes the saved result. See the
[update guide](../esp32/docs/msp430_update.md) for wiring, ESP32 partition-table
requirements, image restrictions, and recovery.

An MSP-FET remains an alternative for direct programming/debugging. Disconnect
it when using the ESP32 as JTAG programmer. The current target configuration
selects TI MSP430 USB1 and MSP430FR5043.

Please refer to the `WULPUS User Manual` of the WULPUS system v 1.2.2 for the instructions on how to flash the MSP430 MCU:
`https://github.com/pulp-bio/wulpus/blob/main/docs/wulpus_user_manual.pdf`

# License
This directory contains third-party sources with their own licenses (primarily
BSD and Apache 2.0). See the respective folders and source headers, including
[`driverlib/license.txt`](wulpus_msp430_firmware/driverlib/license.txt). Firmware
releases must include applicable notices for bundled code and linked runtime
libraries. A compiler change does not remove those dependency obligations.
