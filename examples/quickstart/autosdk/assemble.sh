#!/usr/bin/env bash
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
set -ex

# Run the build script, propagating any command-line arguments
python3 "$SCRIPT_DIR/assemble.py" "$@"
