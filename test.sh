#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
SLEEP_SECONDS=1

usage() {
	cat <<USAGE
Usage: ./test.sh [options] [-- game args...]

Builds with ./build.sh --fast, runs build/jimokomi, then repeats when the game exits.
Stop with Ctrl+C.

Options:
  -j, --jobs N       Use N parallel build jobs. Default: online CPU count.
  --sleep N          Seconds to wait between runs. Default: 1.
  -h, --help         Show this help.
USAGE
}

while (($# > 0)); do
	case "$1" in
		-j|--jobs)
			if (($# < 2)); then
				echo "Missing value for $1" >&2
				exit 2
			fi
			JOBS="$2"
			shift
			;;
		-j*)
			JOBS="${1#-j}"
			;;
		--sleep)
			if (($# < 2)); then
				echo "Missing value for $1" >&2
				exit 2
			fi
			SLEEP_SECONDS="$2"
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		--)
			shift
			break
			;;
		*)
			break
			;;
	esac
	shift
done

if [[ "$JOBS" == "0" || ! "$JOBS" =~ ^[0-9]+$ ]]; then
	echo "Job count must be a positive integer." >&2
	exit 2
fi

cd "$ROOT"

while true; do
	./build.sh --fast -j"$JOBS"
	"$ROOT/build/jimokomi" "$@" || true
	sleep "$SLEEP_SECONDS"
done
