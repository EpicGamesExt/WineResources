#!/usr/bin/env bash
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
set -ex

# Create a Python virtual environment to hold the required dependencies
rm -rf "$SCRIPT_DIR/.venv"
python3 -m venv "$SCRIPT_DIR/.venv"

# Install the Python packages that the build script depends upon
"$SCRIPT_DIR/.venv/bin/python3" -m pip install -r "$SCRIPT_DIR/requirements.txt"

# Run the build script, propagating any command-line arguments
"$SCRIPT_DIR/.venv/bin/python3" "$SCRIPT_DIR/build.py" "$@"
