#!/usr/bin/env bash
set -euo pipefail
tmp_script=$(mktemp)
trap 'rm -f "$tmp_script"' EXIT
curl -fL --retry 3 https://raw.githubusercontent.com/velikiievgeniusultimate/discord-leveler/main/install.sh -o "$tmp_script"
bash "$tmp_script"
