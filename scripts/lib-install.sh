#!/usr/bin/env bash
# Install pinned ESP32 core and Arduino libraries listed in arduino-lib-list.txt.
# Idempotent: arduino-cli skips already-installed versions.
set -euo pipefail

cd "$(dirname "$0")/.."

ESP32_CORE_VERSION="${ESP32_CORE_VERSION:-3.0.5}"
ESP32_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH" >&2
  echo "Install: https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
fi

echo "==> arduino-cli: $(arduino-cli version)"

echo "==> Updating core index (with ESP32 board manager URL)…"
arduino-cli core update-index --additional-urls "$ESP32_INDEX_URL"

echo "==> Installing esp32:esp32@${ESP32_CORE_VERSION}…"
arduino-cli core install "esp32:esp32@${ESP32_CORE_VERSION}" \
  --additional-urls "$ESP32_INDEX_URL"

echo "==> Installing pinned libraries…"
while IFS= read -r raw; do
  line="${raw%%#*}"
  line="${line#"${line%%[![:space:]]*}"}"
  line="${line%"${line##*[![:space:]]}"}"
  [[ -z "$line" ]] && continue
  echo "    $line"
  arduino-cli lib install "$line"
done < arduino-lib-list.txt

echo "==> Done."
