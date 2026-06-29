# WULPUS PRO ESP32 Firmware

This firmware enables WULPUS PRO to connect to Wi-Fi via a ESP32-C6, which enables higher throughputs and thus framerates.

The firmware includes the following features:
- **mDNS**: Multicast DNS for local network discovery
- **Full integration with the Python API**: The firmware also comes with an extension to the Python API, which allows you to control the WULPUS PRO over Wi-Fi
- **Power Management**: The firmware includes power management, which allows the ESP32 to enter light sleep when not in use. This results in ~1.5mA current draw
- **Easy expandability**: The firmware is designed to be easily extensible, allowing you to add new features and functionality as needed

## Getting Started

This firmware is written with [ESP-IDF](https://github.com/espressif/esp-idf). We recommend using at the official [VS Code extension](https://github.com/espressif/vscode-esp-idf-extension/tree/master).

The firmware is tested on the [ESP32-C6-DEVKITM-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitm-1/user_guide.html) board, but should work on any ESP32-C6 board. The firmware is designed to be easily portable to other ESP32 boards as well.

### Board Variants

Board-specific pinouts are stored as ESP-IDF defaults files under `boards/`. Keep reusable project settings in `sdkconfig.defaults`, chip-level settings in `sdkconfig.defaults.<target>`, and board pin choices in the board file.

To configure the firmware for the Seeed Studio XIAO ESP32-C6:

```powershell
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32c6;boards/xiao-esp32c6.defaults" reconfigure
```

To configure the firmware for the ESP32-C6-DEVKITM-1:

```powershell
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32c6;boards/esp32c6-devkitm-1.defaults" reconfigure
```

After reconfiguring, build and flash as usual:

```powershell
idf.py build
idf.py -p COM7 flash monitor
```

### Connection to WULPUS PRO

Use the following default pin mapping to connect the WULPUS PRO acquisition board to the ESP32-C6 board. The XIAO labels follow the Seeed Studio XIAO ESP32-C6 front pin map.

| **Signal**         | **ESP32-C6 GPIO** | **XIAO ESP32-C6 Pin** | **WULPUS PRO Connector Pin** |
|--------------------|-------------------|-----------------------|------------------------------|
| `SPI_SS`           | 21                | D3                    | X3.4                         |
| `SPI_CLK`          | 19                | D8 / SCK              | X3.3                         |
| `SPI_MISO`         | 20                | D9 / MISO             | X3.1                         |
| `SPI_MOSI`         | 18                | D10 / MOSI            | X3.2                         |
| `Data_ready`       | 1                 | D1                    | X4.2                         |
| `BLE_conn_ready`   | 0                 | D0                    | X4.3                         |
| `MSP_RST_N`        | 2                 | D2                    | MSP430 reset                 |

On the XIAO ESP32-C6 side headers shown in the board pinout, the exposed SPI-labeled pins are `D10`/GPIO18 (`MOSI`), `D9`/GPIO20 (`MISO`), and `D8`/GPIO19 (`SCK`). Chip select uses `D3`/GPIO21.
`MSP_RST_N` is configured as an open-drain active-low output without an internal pull-up. It is asserted low while no TCP client is connected, released high after a TCP client connects, and held for 100 ms before command handling continues. It is asserted low again when the TCP connection closes or is lost.

For a new board revision, copy one of the files in `boards/`, change only the `CONFIG_WP_*` and board hardware values, then pass that file in `SDKCONFIG_DEFAULTS`.

### Usage in the Python API

The Python API remains largely unchanged, the only thing you need to do in order to use the Wi-Fi connection is to pass the `WulpusWiFi` communication link to the `WulpusGuiSingleCh` class instead of the `WulpusDongle` class. This will look like this:

```python
from wulpus.wifi import WulpusWiFi

# Create a wifi object
wifi = WulpusWiFi()

# Setup the GUI (uss_conf is already setup and configured)
gui = WulpusGuiSingleCh(wifi, uss_conf)

display(gui)
```

## TODO

This firmware is still a work in progress. The following features are planned for future releases (among others):

- [ ] Add LED status codes
- [ ] Return error codes on the socket on failures

## Licensing

ESP-IDF is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/espressif/esp-idf/blob/master/LICENSE) in the ESP-IDF repository for more information.

ESP-IDF uses multiple third-party components, which are licensed under various other open-source licenses. One example is [FreeRTOS](https://github.com/FreeRTOS). Please make sure all of the licenses are compatible with your project.
