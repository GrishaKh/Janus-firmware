// ApiClient.h — typed wrappers for the Janus device endpoints.
// Phases ship incrementally: P1 added getConfig, P4 adds postEvent;
// /heartbeat and /resync land in P6/P8.
#pragma once

#include <Arduino.h>
#include <vector>

#include "Net.h"

struct ConfigResult {
  bool   ok = false;
  int    status = 0;          // HTTP status, or negative on transport error
  String error;               // populated when !ok

  String device_id;
  String display_name;
  String mode;                // "attendance" | "silent" | "exam" | "maintenance"
  bool   two_factor = false;
  String silent_from;         // "HH:MM:SS" or empty
  String silent_to;           // "HH:MM:SS" or empty
  String alarm_silenced_until;
  String server_time;         // ISO 8601

  size_t              enrolled_count = 0;
  std::vector<String> rfid_uids;     // populated for IdentityCache (P4)
  std::vector<String> admin_phones;  // populated for SmsSender (P7)
  // fingerprint_ids[] / student_codes[] land when v2 fingerprint hardware
  // arrives — skip parsing them for now to keep the JSON pass small.
};

struct PostEventResult {
  bool   ok = false;        // 201, 200-duplicate, or 200-maintenance_dropped
  int    status = 0;
  bool   duplicate = false;
  bool   maintenance_dropped = false;
  String error;
};

struct ResyncResult {
  bool ok = false;          // 200 with parseable summary
  int  status = 0;
  // Per-row outcomes from the server's summary block. Per the API contract
  // accepted_* and duplicates BOTH count as "delivered, free the slot";
  // invalid/errors are dropped (we don't retry per-row failures).
  int  accepted = 0;
  int  duplicates = 0;
  int  maintenance_dropped = 0;
  int  invalid = 0;
  int  errors = 0;
  String error;
};

class ApiClient {
 public:
  explicit ApiClient(Net& net) : _net(net) {}

  ConfigResult    getConfig();
  PostEventResult postEvent(const String& jsonBody);

  // Send up to RESYNC_BATCH_SIZE pre-built event JSON bodies in one batch.
  // The caller wraps the bodies into the `{"events":[...]}` envelope.
  ResyncResult    resync(const String& wrappedBody);

 private:
  Net& _net;
};
