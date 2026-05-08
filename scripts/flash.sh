#!/usr/bin/env bash
# Compile, upload, and open the serial monitor.
# Usage: PORT=/dev/ttyUSB0 ./scripts/flash.sh
set -euo pipefail

cd "$(dirname "$0")/.."

FQBN="${FQBN:-esp32:esp32:esp32:FlashSize=4M,PartitionScheme=min_spiffs}"
PORT="${PORT:-/dev/ttyUSB0}"
BAUD="${BAUD:-115200}"
ESP32_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"
SKETCH_DIR="janus_firmware"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH" >&2
  exit 1
fi

if [[ ! -e "${PORT}" ]]; then
  echo "Serial port ${PORT} does not exist." >&2
  echo "Plug in the ESP32 and try: PORT=<your-port> ./scripts/flash.sh" >&2
  echo "List candidates with: arduino-cli board list" >&2
  exit 1
fi

echo "==> Compiling…"
./scripts/build.sh

echo "==> Uploading to ${PORT}…"
arduino-cli upload \
  --fqbn "${FQBN}" \
  --port "${PORT}" \
  --additional-urls "${ESP32_INDEX_URL}" \
  "${SKETCH_DIR}"

echo "==> Opening serial monitor at ${BAUD} baud (Ctrl-C to exit)…"
arduino-cli monitor --port "${PORT}" --config "baudrate=${BAUD}"
