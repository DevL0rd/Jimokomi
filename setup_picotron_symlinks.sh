#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
desktop_dir="${HOME}/.lexaloffle/Picotron/drive/desktop"
appdata_system_dir="${HOME}/.lexaloffle/Picotron/drive/appdata/system"
folder_link="${desktop_dir}/Jimokomi"
cart_link="${desktop_dir}/Jimokomi_Debug.p64"
cart_target="${repo_root}/Jimokomi_Debug.p64"
startup_link="${appdata_system_dir}/startup.lua"
startup_target="${repo_root}/picotron_startup.lua"

mkdir -p "${desktop_dir}"
mkdir -p "${appdata_system_dir}"

ln -sfn "${repo_root}" "${folder_link}"

if [[ -f "${cart_target}" ]]; then
    ln -sfn "${cart_target}" "${cart_link}"
else
    echo "warning: cart not found at ${cart_target}" >&2
fi

if [[ -f "${startup_target}" ]]; then
    ln -sfn "${startup_target}" "${startup_link}"
else
    echo "warning: startup script not found at ${startup_target}" >&2
fi

echo "folder -> ${folder_link} -> ${repo_root}"
if [[ -f "${cart_target}" ]]; then
    echo "cart   -> ${cart_link} -> ${cart_target}"
fi
if [[ -f "${startup_target}" ]]; then
    echo "startup-> ${startup_link} -> ${startup_target}"
fi
