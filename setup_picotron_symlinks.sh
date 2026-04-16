#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
desktop_dir="${HOME}/.lexaloffle/Picotron/drive/desktop"
folder_link="${desktop_dir}/Jimokomi"
cart_link="${desktop_dir}/Jimokomi_Debug.p64"
cart_target="${repo_root}/Jimokomi_Debug.p64"

mkdir -p "${desktop_dir}"

ln -sfn "${repo_root}" "${folder_link}"

if [[ -f "${cart_target}" ]]; then
    ln -sfn "${cart_target}" "${cart_link}"
else
    echo "warning: cart not found at ${cart_target}" >&2
fi

echo "folder -> ${folder_link} -> ${repo_root}"
if [[ -f "${cart_target}" ]]; then
    echo "cart   -> ${cart_link} -> ${cart_target}"
fi
