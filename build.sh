#!/usr/bin/env bash
# =============================================================================
# crashdiag - one-shot build script.
#
#   ./build.sh                       # stub SDK build (CI syntax check path)
#   ./build.sh --sdk-path /path/sdk  # build against a real HL2SDK (e.g. hl2sdk-dods)
#   ./build.sh --no-clone            # reuse any existing hl2sdk checkout under deps/
#
# The stub build compiles against the bundled headers in sdk/stub/ and needs no
# SDK checkout. A real engine build needs a checked-out HL2SDK whose public/
# tree contains eiface.h, icvar.h, interface.h ...
#
# Output lands in build/.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
DEPS_DIR="${DEPS_DIR:-$SCRIPT_DIR/deps}"
SDK_PATH="${HL2SDK_PATH:-}"
CLONE_DODS=0
SKIP_CLONE=0
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"

while [ $# -gt 0 ]; do
    case "$1" in
        --sdk-path) SDK_PATH="$2"; shift ;;
        --clone-dods) CLONE_DODS=1 ;;
        --no-clone) SKIP_CLONE=1 ;;
        -j|--jobs) JOBS="$2"; shift ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
    shift
done

for tool in cmake git; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: '$tool' is required but not installed." >&2
        exit 1
    fi
done

# --- Real SDK resolution -----------------------------------------------------
# Only consulted when a non-stub build is requested. The alliedmodders/hl2sdk
# mirror carries one branch per game; `dods` is the Day of Defeat: Source SDK
# for the target server.
if [ -z "$SDK_PATH" ] && [ "$CLONE_DODS" = "1" ]; then
    SDK_PATH="$DEPS_DIR/hl2sdk-dods"
    if [ -d "$SDK_PATH/.git" ]; then
        echo "==> reusing existing $SDK_PATH"
    elif [ "$SKIP_CLONE" = "1" ]; then
        echo "error: SDK not found at $SDK_PATH and --no-clone was given." >&2
        exit 1
    else
        echo "==> cloning alliedmodders/hl2sdk (branch dods) -> $SDK_PATH"
        git clone --depth 1 --branch dods \
            https://github.com/alliedmodders/hl2sdk.git "$SDK_PATH"
    fi
fi

if [ -n "$SDK_PATH" ]; then
    if [ ! -f "$SDK_PATH/public/eiface.h" ]; then
        echo "error: $SDK_PATH does not look like an HL2SDK (missing public/eiface.h)." >&2
        exit 1
    fi
    echo "==> real HL2SDK build: $SDK_PATH"
    STUB=OFF
    EXTRA_ARGS=(-DCRASHDIAG_STUB_SDK=OFF -DHL2SDK_PATH="$SDK_PATH")
else
    echo "==> stub SDK build (sdk/stub/)"
    STUB=ON
    EXTRA_ARGS=(-DCRASHDIAG_STUB_SDK=ON)
fi

# --- Configure & build -------------------------------------------------------
echo "==> configuring -> $BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      "${EXTRA_ARGS[@]}"

echo "==> building with $JOBS jobs"
cmake --build "$BUILD_DIR" --parallel "$JOBS"

echo
echo "==> build complete. Artifacts:"
find "$BUILD_DIR" -type f \( -name '*.so' -o -name '*.dll' \) -print 2>/dev/null | sort | sed 's/^/    /'
