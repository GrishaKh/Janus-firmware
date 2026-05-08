#!/usr/bin/env bash
# Compile the Janus firmware sketch via arduino-cli.
# Usage: ./scripts/build.sh [extra arduino-cli flags]
set -euo pipefail

cd "$(dirname "$0")/.."

FQBN="${FQBN:-esp32:esp32:esp32:FlashSize=4M,PartitionScheme=min_spiffs}"
ESP32_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"
SKETCH_DIR="janus_firmware"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH" >&2
  exit 1
fi

# Generate Secrets.h from the template if the developer hasn't yet — placeholder
# values are fine for a compile-only build (they fail at runtime, not compile time).
if [[ ! -f "${SKETCH_DIR}/Secrets.h" ]]; then
  if [[ -f "${SKETCH_DIR}/Secrets.h.example" ]]; then
    echo "==> Secrets.h missing; copying from Secrets.h.example for compile-only run."
    cp "${SKETCH_DIR}/Secrets.h.example" "${SKETCH_DIR}/Secrets.h"
  else
    echo "Secrets.h.example missing — repo is broken." >&2
    exit 1
  fi
fi

echo "==> Compiling ${SKETCH_DIR}/ for ${FQBN}"
arduino-cli compile \
  --fqbn "${FQBN}" \
  --warnings all \
  --additional-urls "${ESP32_INDEX_URL}" \
  "${SKETCH_DIR}" \
  "$@"
