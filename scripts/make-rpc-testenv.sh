#!/usr/bin/env bash
set -euo pipefail

tox -c nexuschat-rpc-client -e py --devenv venv
venv/bin/pip install --upgrade pip
cargo install --locked --path nexuschat-rpc-server/ --root "$PWD/venv" --debug
