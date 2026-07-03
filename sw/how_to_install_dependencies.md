# How to install Python requirements

The software project uses `uv` for Python dependency management.

1. Install `uv`: https://docs.astral.sh/uv/getting-started/installation/
2. Open a terminal in the `sw` folder.
3. Create the local environment and install dependencies:

```bash
uv sync
```

4. Start Jupyter from the same folder:

```bash
uv run jupyter notebook
```

5. Open the required notebook in the browser, for example `wulpus_pro_gui.ipynb` or `wulpus_pro_wifi_example.ipynb`, and follow the notebook instructions.
