// ApiClient.h — typed wrappers for the four Janus device endpoints.
// Phase 1 ships only getConfig(); /events, /heartbeat, /resync land in P4..P8.
#pragma once

#include <Arduino.h>

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

  size_t enrolled_count = 0;
  // P1 keeps the parser tiny: extract scalars + count, don't materialize
  // the rfid/fingerprint/student_code arrays yet. P4 (IdentityCache) will
  // re-parse the same body and populate them.
};

class ApiClient {
 public:
  explicit ApiClient(Net& net) : _net(net) {}

  ConfigResult getConfig();

 private:
  Net& _net;
};
