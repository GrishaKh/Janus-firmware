// Janus-firmware — entry-control system for Armath Arapi.
// Pairs with the backend at https://github.com/GrishaKh/Janus.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <sys/time.h>

#include "ApiClient.h"
#include "AuthGate.h"
#include "Config.h"
#include "IdentityCache.h"
#include "Net.h"
#include "RfidReader.h"
#include "Secrets.h"
#include "TimeKeeper.h"

namespace {
Net            gNet;
ApiClient      gApi(gNet);
TimeKeeper     gTime;
RfidReader     gRfid;
IdentityCache  gIdentity;

// Most recent device.mode from /config; drives AuthGate. Default to the
// safe option until the first poll lands.
String         gMode = "attendance";

// Counter component of `event_id` to disambiguate same-millisecond posts.
uint32_t       gEventSeq = 0;

unsigned long  gNextConfigPollAt = 0;

String mintEventId(const String& deviceId) {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  char buf[64];
  snprintf(buf, sizeof(buf), "%s-%lu%03ld-%lu",
           deviceId.c_str(),
           (unsigned long)tv.tv_sec,
           (long)(tv.tv_usec / 1000),
           (unsigned long)gEventSeq++);
  return String(buf);
}

String buildAuthEventJson(const String& uid,
                          const String& occurredAtIso,
                          const String& eventId) {
  JsonDocument doc;
  doc["type"]        = "auth";
  doc["auth_method"] = "rfid";
  doc["rfid_uid"]    = uid;
  doc["occurred_at"] = occurredAtIso;
  doc["event_id"]    = eventId;
  String out;
  serializeJson(doc, out);
  return out;
}

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
  Serial.print(F("  enrolled     = "));
  Serial.print(cfg.enrolled_count);
  Serial.println(F(" cards"));
  Serial.print(F("  server_time  = ")); Serial.println(cfg.server_time);
  Serial.print(F("  device_time  = ")); Serial.println(gTime.nowIso8601());
  Serial.print(F("  time_sane    = ")); Serial.println(gTime.isTimeSane() ? "yes" : "no");

  if (cfg.server_time.length() > 0) gTime.noteServerTime(cfg.server_time);
  gMode = cfg.mode;
  gIdentity.update(cfg.rfid_uids);
}

void handleCardTap(const String& uid) {
  Serial.print(F("[main] RFID UID: "));
  Serial.println(uid);

  bool known = gIdentity.contains(uid);
  AuthDecision d = AuthGate::decideRfid(known, gMode, gTime.isTimeSane());

  switch (d) {
    case AuthDecision::NoOp:
      Serial.println(F("[main] gate: NoOp (time/mode)"));
      return;
    case AuthDecision::RejectedAuth:
      // P5 will POST a breach here. P4 only ships the happy path.
      Serial.println(F("[main] gate: RejectedAuth (P5 will post breach)"));
      return;
    case AuthDecision::LogAuth:
      break;
  }

  String occurredAt = gTime.nowIso8601();
  String eventId    = mintEventId(DEVICE_ID);
  String body       = buildAuthEventJson(uid, occurredAt, eventId);

  Serial.print(F("[main] POST /events  event_id="));
  Serial.println(eventId);

  PostEventResult r = gApi.postEvent(body);
  if (r.ok) {
    Serial.print(F("[main] auth logged"));
    if (r.duplicate) Serial.print(F(" (duplicate, server already had it)"));
    Serial.println();
    // Brief LED double-blink for "accepted".
    for (int i = 0; i < 2; ++i) {
      digitalWrite(PIN_STATUS_LED, LOW);  delay(60);
      digitalWrite(PIN_STATUS_LED, HIGH); delay(60);
    }
  } else {
    Serial.print(F("[main] auth post failed: "));
    Serial.println(r.error);
    // P6 will buffer this in the ring; for now the event is dropped.
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

  gTime.begin();
  gIdentity.begin();
  gRfid.begin();

  // Kick the first /config fetch immediately, then on the configured cadence.
  pollConfig();
  gNextConfigPollAt = millis() + CONFIG_POLL_INTERVAL_MS;
}

void loop() {
  if (gNet.needsProvisioning()) {
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

  String uid;
  if (gRfid.poll(uid)) {
    handleCardTap(uid);
  } else {
    digitalWrite(PIN_STATUS_LED, gNet.isOnline() ? HIGH : LOW);
  }

  delay(50);
}
