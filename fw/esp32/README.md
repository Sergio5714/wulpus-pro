# WULPUS PRO source files for ESP32 firmware project

This firmware enables WULPUS PRO to connect to Wi-Fi via a ESP32-C6, which enables higher throughputs and thus framerates.

The firmware includes the following features:
- **mDNS**: Multicast DNS for local network discovery
- **Full integration with the Python API**: The firmware also comes with an extension to the Python API, which allows you to control the WULPUS PRO over Wi-Fi
- **Power Management**: The firmware includes power management, which allows the ESP32 to enter light sleep when not in use. This results in ~1.5mA current draw
- **Easy expandability**: The firmware is designed to be easily extensible, allowing you to add new features and functionality as needed

# How to get started?

This firmware is written with [ESP-IDF](https://github.com/espressif/esp-idf). We recommend using at the official [VS Code extension](https://github.com/espressif/vscode-esp-idf-extension/tree/master).

The firmware has been tested on both the [ESP32-C6-DEVKITM-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitm-1/user_guide.html) and the [Seeed Studio XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/) boards. The XIAO ESP32-C6 target is expected to receive longer-term support, while the DevKit remains useful for development and bring-up. The firmware should work on other ESP32-C6 boards with suitable board defaults, and is designed to be portable to other ESP32 boards as well.

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

Adapt `COM7` to the serial port assigned to your ESP32 board.

### IMPORTANT: Provision Wi-Fi after flashing

After flashing the firmware for the first time, provision the ESP32-C6 with the Wi-Fi network credentials. Provisioning lets the ESP32 store the SSID and password in non-volatile memory, so the firmware can reconnect to the same network after reset or power cycling.

Follow Espressif's provisioning documentation for the first-time setup flow: [ESP-IDF Provisioning API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/provisioning/index.html).

### Connection to WULPUS PRO

Use the following default pin mappings to connect the WULPUS PRO acquisition board to the ESP32-C6 board.

Seeed Studio XIAO ESP32-C6:

| **Signal**         | **ESP32-C6 GPIO** | **XIAO ESP32-C6 Pin** | **WULPUS PRO Connector Pin** |
|--------------------|-------------------|-----------------------|------------------------------|
| `SPI_SS`           | 21                | D3                    | X3.4                         |
| `SPI_CLK`          | 19                | D8 / SCK              | X3.3                         |
| `SPI_MISO`         | 20                | D9 / MISO             | X3.1                         |
| `SPI_MOSI`         | 18                | D10 / MOSI            | X3.2                         |
| `Data_ready`       | 1                 | D1                    | X4.2                         |
| `BLE_conn_ready`   | 0                 | D0                    | X4.3                         |
| `MSP_RST_N`        | 2                 | D2                    | X1.5                         |

ESP32-C6-DEVKITM-1:

| **Signal**         | **ESP32-C6 GPIO** | **DevKit Header Name** | **WULPUS PRO Connector Pin** |
|--------------------|-------------------|------------------------|------------------------------|
| `SPI_SS`           | 18                | 18                     | X3.4                         |
| `SPI_CLK`          | 6                 | 6                      | X3.3                         |
| `SPI_MISO`         | 7                 | 7                      | X3.1                         |
| `SPI_MOSI`         | 2                 | 2                      | X3.2                         |
| `Data_ready`       | 1                 | 1/N                    | X4.2                         |
| `BLE_conn_ready`   | 0                 | 0/N                    | X4.3                         |
| `MSP_RST_N`        | 3                 | 3                      | X1.5                         |

For the ESP32-C6-DEVKITM-1 defaults, GPIO2 is used for `SPI_MOSI`; `MSP_RST_N` is therefore mapped to GPIO3.

On the XIAO ESP32-C6 side headers shown in the board pinout, the exposed SPI-labeled pins are `D10`/GPIO18 (`MOSI`), `D9`/GPIO20 (`MISO`), and `D8`/GPIO19 (`SCK`). Chip select uses `D3`/GPIO21.
`MSP_RST_N` is configured as an open-drain active-low output without an internal pull-up. It is asserted low while no TCP client is connected, released high after a TCP client connects, and held for 100 ms before command handling continues. It is asserted low again when the TCP connection closes or is lost.

For a new board revision, copy one of the files in `boards/`, change only the `CONFIG_WP_*` and board hardware values, then pass that file in `SDKCONFIG_DEFAULTS`.

### Python Wi-Fi Example

For Python-side usage, see the Wi-Fi example notebook at `../../sw/wulpus_pro_wifi_example.ipynb`. It shows device discovery, connection setup with `WulpusWiFi`, configuration transfer, and data acquisition over the TCP link.

# License

ESP-IDF is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/espressif/esp-idf/blob/master/LICENSE) in the ESP-IDF repository for more information.

ESP-IDF uses multiple third-party components, which are licensed under various other open-source licenses. One example is [FreeRTOS](https://github.com/FreeRTOS). Please make sure all of the licenses are compatible with your project.
