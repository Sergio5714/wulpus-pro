# WULPUS PRO source files for ESP32 firmware project

This firmware connects WULPUS PRO to a host through either Wi-Fi TCP or the
native USB CDC channel of an ESP32-C6.

The firmware includes the following features:
- **mDNS**: Multicast DNS for local network discovery
- **Native USB CDC**: Wired binary communication through the same XIAO USB connector used for flashing and JTAG
- **Full integration with the Python API**: The firmware also comes with an extension to the Python API, which allows you to control the WULPUS PRO over Wi-Fi
- **Power Management**: The firmware includes power management, which allows the ESP32 to enter light sleep when not in use. This results in ~1.5mA current draw
- **Easy expandability**: The firmware is designed to be easily extensible, allowing you to add new features and functionality as needed

## Communication architecture

TCP and USB CDC carry the same framed WULPUS binary protocol. Command parsing,
MSP430 startup/shutdown, SPI configuration, and RF-frame generation are shared;
transport-specific code is restricted to reading and writing an ordered byte
stream.

The transport implementation is organized as follows:

- `components/wulpus_transport/wulpus_transport.*` defines exact reads,
  complete writes, header prefetching, and packet-level transmit locking.
- `wulpus_tcp_transport.*` adapts the existing socket component.
- `wulpus_usb_transport.*` adapts the ESP-IDF USB Serial/JTAG CDC driver.
- `components/commander` implements the shared binary command codec.
- `main/app_main.c` owns arbitration, the common session lifecycle, MSP430 SPI
  access, and RF-frame routing.

### Session arbitration

The firmware supports one controlling host at a time. TCP and USB listener
tasks wait concurrently, but neither owns WULPUS PRO until it receives a valid
protocol header. Consequently, an attached USB cable used only for power,
flashing, or JTAG does not block Wi-Fi.

The first listener to acquire the session mutex becomes the owner. A competing
client receives the `BUSY` protocol response. Only the owner can issue commands
or receive `GET_DATA` frames. Ownership is released after `CLOSE`, transport
failure, or reset. Common cleanup stops transmission, returns the MSP430 to its
safe state, asserts MSP430 reset, and then allows either listener to win the
next session.

The USB listener starts as soon as provisioning is launched and does not wait
for Wi-Fi provisioning or association to finish. USB CDC therefore remains
available when the network is unconfigured, unavailable, or reconnecting.

Command acknowledgements and RF data originate from different FreeRTOS tasks.
The active transport therefore has a TX mutex held across each complete header
and payload so that packets cannot interleave.

### USB CDC, flashing, JTAG, and logs

ESP32-C6 has a fixed USB Serial/JTAG composite device. Its CDC channel carries
WULPUS binary data in normal application mode, while its JTAG interface remains
available to OpenOCD. Close the Python GUI before running `idf.py -p COMx flash`
so the flashing tool can claim the CDC port. After flashing and reset, reopen
the same port in the GUI.

Application logs must not share USB CDC with binary data. The default ESP32-C6
configuration therefore disables the application console. The XIAO pinout uses
the default UART0 pins for SPI, so a debug build may enable only a custom UART
console mapped to verified, non-conflicting pins. ROM or bootloader text can
still appear briefly around reset; host code discards stale input and
resynchronizes on the six-byte `wulpus` magic value.

The firmware acquires an `ESP_PM_NO_LIGHT_SLEEP` power-management lock before
automatic light sleep can interrupt USB enumeration. The USB listener retains
that lock whenever host SOF packets are detected and releases it only after USB
disconnection. This keeps enumeration and the wired byte stream responsive,
while still allowing light sleep when no USB host is present.

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

Use the following pin mapping for the integrated WULPUS PRO Wi-Fi host PCB.

Seeed Studio XIAO ESP32-C6:

| **Signal**         | **ESP32-C6 GPIO** | **XIAO ESP32-C6 Pin** | **WULPUS PRO Connector Pin** |
|--------------------|-------------------|-----------------------|------------------------------|
| `SPI_SS`           | 17                | D7                    | X3.4                         |
| `SPI_CLK`          | 18                | D10                   | X3.3                         |
| `SPI_MISO`         | 20                | D9 / MISO             | X3.1                         |
| `SPI_MOSI`         | 19                | D8 / SCK              | X3.2                         |
| `Data_ready`       | 1                 | D1                    | X4.2                         |
| `MSP_RST_N`        | 0                 | D0                    | X1.5                         |

ESP32-C6-DEVKITM-1:

| **Signal**         | **ESP32-C6 GPIO** | **DevKit Header Name** | **WULPUS PRO Connector Pin** |
|--------------------|-------------------|------------------------|------------------------------|
| `SPI_SS`           | 18                | 18                     | X3.4                         |
| `SPI_CLK`          | 6                 | 6                      | X3.3                         |
| `SPI_MISO`         | 7                 | 7                      | X3.1                         |
| `SPI_MOSI`         | 2                 | 2                      | X3.2                         |
| `Data_ready`       | 1                 | 1/N                    | X4.2                         |
| `MSP_RST_N`        | 3                 | 3                      | X1.5                         |

For the ESP32-C6-DEVKITM-1 defaults, GPIO2 is used for `SPI_MOSI`; `MSP_RST_N` is therefore mapped to GPIO3.

The WULPUS PRO WiFi host PCB routing intentionally differs from the SPI functions printed on the XIAO headers. Use the GPIO assignments above rather than the header function names.

The dedicated `BLE_conn_ready`/host-ready signal used by the earlier wired nRF52 setup is not used. `MSP_RST_N` is configured as an open-drain active-low output without an internal pull-up. It is asserted while no transport owns a session, released after the first valid TCP or USB command claims one, and allowed 100 ms for MSP430 boot before command handling continues. Before normal session closure or reset, the ESP32 sends the MSP430 restart command and waits for the next configuration-request edge, which confirms that the acquisition loop returned and `disableAll()` ran. It then asserts reset. A bounded timeout and hard communication-error path retain immediate reset as an emergency fallback. The matching MSP430 firmware therefore starts configuration and acquisition without polling a link-ready pin.

For a new board revision, copy one of the files in `boards/`, change only the `CONFIG_WP_*` and board hardware values, then pass that file in `SDKCONFIG_DEFAULTS`.

### Python communication example

For Python-side usage, see `../../sw/wulpus_pro_wifi_example.ipynb`. Its GUI
offers Wi-Fi TCP, native USB CDC, and BLE transports, followed by detailed
manual Wi-Fi protocol steps.

# Authors

- Cedric Hirschi, ETH Zurich
- Sergei Vistrikov (Sergio5714 on GitHub)

# License

ESP-IDF is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/espressif/esp-idf/blob/master/LICENSE) in the ESP-IDF repository for more information.

ESP-IDF uses multiple third-party components, which are licensed under various other open-source licenses. One example is [FreeRTOS](https://github.com/FreeRTOS). Please make sure all of the licenses are compatible with your project.
