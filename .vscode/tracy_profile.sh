#!/usr/bin/env bash
set -eu

mode="${1:-}"
workspace="${2:-}"
seconds="${3:-15}"
game_bin="${4:-}"

if [ -z "$mode" ] || [ -z "$workspace" ]; then
    echo "usage: tracy_profile.sh <prepare|finalize|run> <workspace> [seconds] [game_bin]" >&2
    exit 1
fi

captures_dir="$workspace/captures"
capture_bin="$workspace/build/profile/tracy-capture/tracy-capture"
csvexport_bin="$workspace/build/profile/tracy-csvexport/tracy-csvexport"
capture_file="$captures_dir/latest.tracy"
capture_pid_file="$captures_dir/.latest-capture.pid"
capture_log_file="$captures_dir/.latest-capture.log"

export_capture() {
    mkdir -p "$captures_dir"

    if [ ! -s "$capture_file" ]; then
        echo "No Tracy capture file created at $capture_file" >&2
        exit 1
    fi

    "$csvexport_bin" "$capture_file" > "$captures_dir/latest-zones.csv"
    "$csvexport_bin" -e "$capture_file" > "$captures_dir/latest-zones-self.csv"
    "$csvexport_bin" -f CorePhys "$capture_file" > "$captures_dir/latest-corephys-zones.csv"
    "$csvexport_bin" -f renderer "$capture_file" > "$captures_dir/latest-renderer-zones.csv"
    "$csvexport_bin" -f Sim "$capture_file" > "$captures_dir/latest-sim-zones.csv"
    "$csvexport_bin" -m "$capture_file" > "$captures_dir/latest-messages.csv"
}

prepare_capture() {
    mkdir -p "$captures_dir"
    rm -f "$capture_file" "$capture_pid_file" "$capture_log_file"

    nohup "$capture_bin" -f -a 127.0.0.1 -o "$capture_file" -s "$seconds" > "$capture_log_file" 2>&1 &
    echo "$!" > "$capture_pid_file"
}

finalize_capture() {
    if [ -f "$capture_pid_file" ]; then
        capture_pid="$(cat "$capture_pid_file")"
        while kill -0 "$capture_pid" 2>/dev/null; do
            sleep 1
        done
        rm -f "$capture_pid_file"
    fi

    export_capture
}

case "$mode" in
    prepare)
        prepare_capture
        ;;
    finalize)
        finalize_capture
        ;;
    run)
        if [ -z "$game_bin" ]; then
            echo "run mode requires a game binary path" >&2
            exit 1
        fi

        cleanup() {
            finalize_capture
        }

        trap cleanup EXIT
        prepare_capture
        "$game_bin"
        ;;
    *)
        echo "unknown mode: $mode" >&2
        exit 1
        ;;
esac
