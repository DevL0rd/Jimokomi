#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT/build"
BUILD_TYPE="Release"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
ENABLE_LTO=0
ENABLE_NATIVE=0
CLEAN=0

usage() {
	cat <<USAGE
Usage: ./build.sh [options]

Options:
  --debug            Build with CMAKE_BUILD_TYPE=Debug.
  --release          Build with CMAKE_BUILD_TYPE=Release. Default.
  --lto              Enable link-time optimization.
  --native           Add -march=native -mtune=native for this CPU.
  --fast             Enable --lto and --native.
  --clean            Remove the build directory before configuring.
  -j, --jobs N       Use N parallel build jobs.
  -h, --help         Show this help.
USAGE
}

while (($# > 0)); do
	case "$1" in
		--debug)
			BUILD_TYPE="Debug"
			;;
		--release)
			BUILD_TYPE="Release"
			;;
		--lto)
			ENABLE_LTO=1
			;;
		--native)
			ENABLE_NATIVE=1
			;;
		--fast)
			ENABLE_LTO=1
			ENABLE_NATIVE=1
			;;
		--clean)
			CLEAN=1
			;;
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
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage >&2
			exit 2
			;;
	esac
	shift
done

if [[ "$JOBS" == "0" || ! "$JOBS" =~ ^[0-9]+$ ]]; then
	echo "Job count must be a positive integer." >&2
	exit 2
fi

CMAKE_ARGS=(
	-S "$ROOT"
	-B "$BUILD_DIR"
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if ((ENABLE_LTO)); then
	CMAKE_ARGS+=(-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON)
else
	CMAKE_ARGS+=(-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF)
fi

if ((ENABLE_NATIVE)); then
	CMAKE_ARGS+=(-DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG -march=native -mtune=native")
fi

if ((CLEAN)); then
	rm -rf "$BUILD_DIR"
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" -j"$JOBS"

echo "Built $BUILD_DIR/jimokomi"
