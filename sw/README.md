# WULPUS PRO software
This directory contains the WULPUS PRO Python API, graphical user interface components, Wi-Fi and serial transports, and example Jupyter notebooks.

The supported configuration implementation is the WULPUS PRO stack:

- `wulpus/rx_tx_conf_pro.py`: 16-channel TX/RX mask generation
- `wulpus/uss_conf_pro.py`: acquisition configuration and packet encoding
- `wulpus/config_package_pro.py`: configuration field definitions
- `wulpus/uss_conf_gui_pro.py`: configuration widgets
- `wulpus/wifi_link.py`: robust TCP transport and acquisition lifecycle
- `wulpus/wifi_discovery.py`: mDNS discovery

The generic `gui.py` module and the `ble_dongle.py` transport are retained because WULPUS PRO notebooks use them for acquisition display and BLE-dongle serial-host compatibility.
`WulpusGuiSingleCh` depends on the `WulpusProCommunicationLink` protocol rather than a concrete transport, so it can operate with either `WulpusBleDongle` or `WulpusProWiFiLink`.

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
