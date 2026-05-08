#!/usr/bin/env bash
# build.sh — configure + build PitchEdit VST3 (Linux)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# ── Dependencies check ────────────────────────────────────────────────────────
MISSING=()
for cmd in cmake git ninja-build; do
    command -v "${cmd%%-*}" &>/dev/null || MISSING+=("$cmd")
done
if (( ${#MISSING[@]} )); then
    echo "Install missing packages: sudo apt install ${MISSING[*]}"
    exit 1
fi

# ── Configure ─────────────────────────────────────────────────────────────────
cmake -B "${BUILD_DIR}" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      "${SCRIPT_DIR}"

# ── Build ─────────────────────────────────────────────────────────────────────
cmake --build "${BUILD_DIR}" --parallel

echo ""
echo "Build complete."
echo "VST3 bundle: ${BUILD_DIR}/PitchEdit_artefacts/Release/VST3/PitchEdit.vst3"
echo "Copy to ~/.vst3/ to install:"
echo "  cp -r ${BUILD_DIR}/PitchEdit_artefacts/Release/VST3/PitchEdit.vst3 ~/.vst3/"
