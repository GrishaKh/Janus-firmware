// AuthGate.h — pure decision logic for "what should this card-tap do?".
// No I/O, no globals; the caller passes everything in. That keeps it
// trivially unit-testable and makes the maintenance/2FA branches easy to
// reason about as more inputs land in v2.
#pragma once

#include <Arduino.h>

enum class AuthDecision {
  NoOp,         // ignore (server would drop, or device clock is unsane)
  LogAuth,      // known card → POST /events as `auth`
  RejectedAuth  // unknown card → POST /events as `breach reason=rejected_auth`
};

class AuthGate {
 public:
  // `mode` mirrors the server's device.mode: "attendance" | "silent" |
  // "exam" | "maintenance". `timeSane` is TimeKeeper::isTimeSane() — events
  // with a bogus occurred_at would either be rejected or stored with a
  // wrong timestamp, neither of which is desirable.
  static AuthDecision decideRfid(bool          identityKnown,
                                 const String& mode,
                                 bool          timeSane);
};
