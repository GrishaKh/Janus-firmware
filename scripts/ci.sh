#!/usr/bin/env bash
# Headless CI entry point. Installs the toolchain and compiles the sketch.
# Used by .github/workflows/ci.yml; run locally to reproduce CI behavior.
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/lib-install.sh
./scripts/build.sh
