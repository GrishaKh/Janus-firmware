// Janus-firmware — entry-control system for Armath Arapi.
// Pairs with the backend at https://github.com/GrishaKh/Janus.
//
// Phase 0 baseline: empty setup() / loop() that proves the build chain
// works end-to-end (arduino-cli compile + GitHub Actions CI). Real logic
// lands phase by phase — see ../README.md and the plan file.

#include <Arduino.h>
#include "Config.h"

void setup() {
  Serial.begin(115200);
  // Give the host serial monitor a moment to attach before the first line.
  delay(200);
  Serial.println();
  Serial.print(F("Janus firmware "));
  Serial.print(F(JANUS_FW_VERSION));
  Serial.println(F(" — boot"));
}

void loop() {
  delay(1000);
}
