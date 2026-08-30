# WULPUS PRO software
This directory contains the WULPUS PRO Python API, graphical user interface components, Wi-Fi and serial transports, and example Jupyter notebooks.

The supported configuration implementation is the WULPUS PRO stack:

- `wulpus/rx_tx_conf_pro.py`: 16-channel TX/RX mask generation
- `wulpus/uss_conf_pro.py`: acquisition configuration and packet encoding
- `wulpus/config_package_pro.py`: configuration field definitions
- `wulpus/uss_conf_gui_pro.py`: configuration widgets
- `wulpus/wifi_link.py`: robust TCP transport and acquisition lifecycle
- `wulpus/wifi_discovery.py`: mDNS discovery
- `wulpus/usb_cdc_link.py`: native ESP32-C6 USB CDC transport

The generic `gui.py` module and the `ble_dongle.py` transport are retained because WULPUS PRO notebooks use them for acquisition display and BLE-dongle serial-host compatibility.
`WulpusGuiSingleCh` depends on the `WulpusProCommunicationLink` protocol rather than a concrete transport, so it can operate with `WulpusBleDongle`, `WulpusProWiFiLink`, or `WulpusProUsbCdcLink`.

## USB CDC operation

Select **USB CDC** in the example GUI when the XIAO ESP32-C6 is connected
directly to the PC. The link scans for the Espressif USB Serial/JTAG CDC port
and runs the same WULPUS command and RF-frame protocol used by Wi-Fi.

Only one application can own the COM port. Before flashing or opening IDF
Monitor, stop acquisition and close the device in the GUI. Flash normally with:

```powershell
idf.py -p COM9 flash
```

Replace `COM9` with the detected port. Reopen it in the GUI after the ESP32
restarts. JTAG is a separate interface of the ESP32-C6 composite USB device and
remains available, although halting the CPU interrupts live acquisition.

### USB frame-rate profiling

Close the GUI and all serial monitors, then run the standalone profiler with
explicit acquisition periods. TX is disabled by default, so this exercises the
capture and USB transport without firing the pulser:

```powershell
uv run python profile_usb_fps.py --port COM9 --period-us 10000 5000 2000 --frames 500
```

The profiler reports target and measured FPS, 95th-percentile frame latency,
timeouts, and acquisition-number gaps. The fastest passing period is the
maximum stable rate among the values tested. The `simplified` preset disables
TX/RX masks and uses a +1 ms DC-DC margin. The `active-all` preset selects all
channels and fixes DC-DC turn-on at 100 us. Custom and saved configurations use
their explicitly selected timing and masks. Enabling TX requires an
independently verified safe pulse-repetition rate for the connected hardware
and transducer.

To reproduce a configuration saved by the acquisition GUI, pass its JSON file.
The saved measurement period is used when `--period-us` is omitted:

```powershell
uv run python profile_usb_fps.py --port COM10 --config uss_config_pro.json --frames 2000
```

Specify `--period-us` to override only the saved period for a rate sweep. New
GUI configuration files include TX/RX mask arrays. Legacy files without masks
remain compatible and use one zero-mask TX/RX configuration unless masks or a
profiler mode are supplied explicitly.

### Runtime status

The TCP and USB CDC links expose the same ESP32 diagnostic API:

```python
status = link.get_status()
print(hex(status.error_flags), status.current_buffer_usage)

# Clear all sticky errors while preserving lifetime counters.
link.clear_status()

# Clear sticky errors and reset diagnostic counters.
link.clear_status(clear_counters=True)
```

The status snapshot includes acquisition-buffer overflow, SPI and link errors,
DATA_READY/SPI/transmitted-frame counters, discarded frames, and current and
maximum DMA-buffer occupancy.

# How to get started?

Install dependencies with `uv` and launch Jupyter from this folder:

```bash
uv sync
uv run jupyter notebook
```

For more details, see `sw/how_to_install_dependencies.md`.

## Wi-Fi example on Windows

The `wulpus_pro_wifi_example.ipynb` notebook discovers the ESP32 through the
`_wulpus._tcp.local.` mDNS service and communicates with it over TCP port 2121.
The computer and WULPUS PRO must be connected to the same local network.

Windows may block mDNS or the TCP connection when the network profile is
**Public**. On a trusted network, either change its profile to **Private**, or
open PowerShell as Administrator and add narrowly scoped rules for the Python
interpreter used by this project's virtual environment:

```powershell
# Run from the repository's sw directory.
$pythonPath = (Resolve-Path ".\.venv\Scripts\python.exe").Path

New-NetFirewallRule `
    -DisplayName "WULPUS PRO mDNS discovery" `
    -Direction Inbound `
    -Action Allow `
    -Program $pythonPath `
    -Protocol UDP `
    -LocalPort 5353 `
    -RemoteAddress LocalSubnet `
    -Profile Public

New-NetFirewallRule `
    -DisplayName "WULPUS PRO TCP communication" `
    -Direction Outbound `
    -Action Allow `
    -Program $pythonPath `
    -Protocol TCP `
    -RemotePort 2121 `
    -RemoteAddress LocalSubnet `
    -Profile Public
```

The active notebook interpreter can be checked with:

```python
import sys
print(sys.executable)
```

After changing the network profile or firewall rules, restart the notebook
kernel and reset the ESP32. If multicast service discovery remains unavailable,
`WulpusProWiFiLink.get_available()` also attempts to resolve `wulpus.local`.
The ESP32 IP address can additionally be supplied manually using
`WulpusProWiFiDevice` as shown in the example notebook.

To remove the firewall rules later:

```powershell
Remove-NetFirewallRule -DisplayName "WULPUS PRO mDNS discovery"
Remove-NetFirewallRule -DisplayName "WULPUS PRO TCP communication"
```

# License
The source files are released under Apache v2.0 (`Apache-2.0`) license unless noted otherwise, please refer to the `sw/LICENSE` file for details.
