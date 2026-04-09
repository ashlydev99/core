#!/usr/bin/env bash
set -euo pipefail
cargo install --locked --path nexuschat-rpc-server/ --root "$PWD/venv" --debug
PATH="$PWD/venv/bin:$PATH" tox -c nexuschat-rpc-client
