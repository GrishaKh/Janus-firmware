// Janus-firmware — entry-control system for Armath Arapi.
// Pairs with the backend at https://github.com/GrishaKh/Janus.
//
// Phase 1 milestone: WiFi + TLS + bearer auth, prove the connection by
// pulling /api/attendance/config and printing the parsed scalar fields.
// No timekeeping, no card reader, no event posting yet.

#include <Arduino.h>

#include "ApiClient.h"
#include "Config.h"
#include "Net.h"
#include "RfidReader.h"
#include "TimeKeeper.h"

namespace {
Net          gNet;
ApiClient    gApi(gNet);
TimeKeeper   gTime;
RfidReader   gRfid;

// Pull /config every CONFIG_POLL_INTERVAL_MS so the dashboard stays
// reflective of latest state. P4 will hook the result into IdentityCache.
unsigned long gNextConfigPollAt = 0;

void pollConfig() {
  ConfigResult cfg = gApi.getConfig();
  if (!cfg.ok) {
    Serial.print(F("[main] getConfig failed: "));
    Serial.println(cfg.error);
    return;
  }
  Serial.println(F("[main] /config:"));
  Serial.print(F("  device_id    = ")); Serial.println(cfg.device_id);
  Serial.print(F("  display_name = ")); Serial.println(cfg.display_name);
  Serial.print(F("  mode         = ")); Serial.println(cfg.mode);
  Serial.print(F("  two_factor   = ")); Serial.println(cfg.two_factor ? "true" : "false");
  Serial.print(F("  silent       = "));
  Serial.print(cfg.silent_from.length() > 0 ? cfg.silent_from : "(none)");
  Serial.print(F(" .. "));
  Serial.println(cfg.silent_to.length() > 0 ? cfg.silent_to : "(none)");
  Serial.print(F("  enrolled     = "));
  Serial.print(cfg.enrolled_count);
  Serial.println(F(" cards"));
  Serial.print(F("  server_time  = ")); Serial.println(cfg.server_time);
  Serial.print(F("  device_time  = ")); Serial.println(gTime.nowIso8601());
  Serial.print(F("  time_sane    = ")); Serial.println(gTime.isTimeSane() ? "yes" : "no");

  if (cfg.server_time.length() > 0) {
    gTime.noteServerTime(cfg.server_time);
  }
}
}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.print(F("Janus firmware "));
  Serial.print(F(JANUS_FW_VERSION));
  Serial.println(F(" — boot"));

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  gNet.begin();
  if (!gNet.isOnline()) {
    Serial.println(F("[main] WiFi not up after begin() — will retry on next poll"));
  }

  // Time has to come up after WiFi (so NTP can sync) but before any event
  // POSTs would happen. P1's loop has no event POSTs yet, so order is fine.
  gTime.begin();

  gRfid.begin();

  // Kick the first /config fetch immediately, then on the configured cadence.
  pollConfig();
  gNextConfigPollAt = millis() + CONFIG_POLL_INTERVAL_MS;
}

void loop() {
  if (gNet.needsProvisioning()) {
    // 3-pulse LED cycle every 2.5 s, no posts.
    static const uint16_t kPulse[] = {120, 120, 120, 120, 120, 1900};
    for (size_t i = 0; i < sizeof(kPulse) / sizeof(kPulse[0]); ++i) {
      digitalWrite(PIN_STATUS_LED, (i % 2 == 0) ? HIGH : LOW);
      delay(kPulse[i]);
    }
    return;
  }

  if ((long)(millis() - gNextConfigPollAt) >= 0) {
    pollConfig();
    gNextConfigPollAt = millis() + CONFIG_POLL_INTERVAL_MS;
  }

  // RFID is the fastest-changing input — poll it every loop tick so a card
  // tap registers within ~50 ms. P3 just prints; P4 will route through
  // IdentityCache and AuthGate.
  String uid;
  if (gRfid.poll(uid)) {
    Serial.print(F("[main] RFID UID: "));
    Serial.print(uid);
    Serial.print(F("  ("));
    Serial.print(uid.length() / 2);
    Serial.println(F(" bytes)"));
    digitalWrite(PIN_STATUS_LED, LOW);
    delay(60);
    digitalWrite(PIN_STATUS_LED, HIGH);
  } else {
    digitalWrite(PIN_STATUS_LED, gNet.isOnline() ? HIGH : LOW);
  }

  delay(50);
}
