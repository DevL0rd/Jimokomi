#!/usr/bin/env bash
set -euo pipefail

disable_flag="${HOME}/.lexaloffle/Picotron/drive/appdata/system/disable_jimokomi_autostart.txt"
rm -f "${disable_flag}"
echo "enabled Jimokomi Picotron autostart"
