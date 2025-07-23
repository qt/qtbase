# Qt CycloneDX SBOM helper tool

This repository contains a Python script to create a CycloneDX Software
Bill of Materials (SBOM) document for Qt-based projects.

## Requirements

- [Python 3.9](https://www.python.org/downloads/),
- `pip` to manage Python packages.
- [cyclonedx-python-lib package](https://pypi.org/project/cyclonedx-python-lib/) can be installed via pip
- optional [tomli package](https://pypi.org/project/tomli/) if using Python 3.9 and the vendored
  tomli package is causing issues

```sh
pip install 'cyclonedx-python-lib[json-validation]' tomli
```
or
```
pip install . # in the current directory to parse the `pyproject.toml` dependencies
```

## Description

The tool is not meant to be run standalone. Instead, the Qt CMake build system generates an
intermediate TOML file during the build process, which is then processed by the provided Python
script to create the final CycloneDX SBOM in JSON format.

## Development

The script is using [uv](https://docs.astral.sh/uv/getting-started/installation/) to manage
a virtual environment for development. Install it before running the commands below.

Run

```sh
# Create a virtual environment with with the project dependencies using `uv` in the current dir
make env

# Optionally install the package in editable mode, for IDE support if needed.
uv pip install -e .

# Run the script
python ./qt_cyclonedx_generator/qt_cyclonedx_generator.py
```

To format the source code run:
```sh
make format # uses uvx ruff format
```

To run the lints:
```sh
make lint # uses uvx ruff check and pyright
```
