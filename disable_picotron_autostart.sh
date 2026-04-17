#!/usr/bin/env bash
set -euo pipefail

disable_flag="${HOME}/.lexaloffle/Picotron/drive/appdata/system/disable_jimokomi_autostart.txt"
mkdir -p "$(dirname "${disable_flag}")"
touch "${disable_flag}"
echo "disabled Jimokomi Picotron autostart"
