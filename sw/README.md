# WULPUS PRO software
This directory contains the WULPUS PRO Python API, graphical user interface components, Wi-Fi and serial transports, and example Jupyter notebooks.

The supported configuration implementation is the WULPUS PRO stack:

- `wulpus/rx_tx_conf_pro.py`: 16-channel TX/RX mask generation
- `wulpus/uss_conf_pro.py`: acquisition configuration and packet encoding
- `wulpus/config_package_pro.py`: configuration field definitions
- `wulpus/uss_conf_gui_pro.py`: configuration widgets
- `wulpus/wifi.py` and `wulpus/scanner.py`: Wi-Fi transport and discovery

The generic `gui.py` and `dongle.py` modules are retained because WULPUS PRO notebooks use them for acquisition display and serial-host compatibility.

# How to get started?

Install dependencies with `uv` and launch Jupyter from this folder:

```bash
uv sync
uv run jupyter notebook
```

For more details, see `sw/how_to_install_dependencies.md`.

# License
The source files are released under Apache v2.0 (`Apache-2.0`) license unless noted otherwise, please refer to the `sw/LICENSE` file for details.
