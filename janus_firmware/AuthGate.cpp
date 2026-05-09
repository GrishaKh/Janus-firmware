#include "AuthGate.h"

AuthDecision AuthGate::decideRfid(bool          identityKnown,
                                  const String& mode,
                                  bool          timeSane) {
  if (!timeSane)              return AuthDecision::NoOp;
  if (mode == "maintenance")  return AuthDecision::NoOp;
  if (identityKnown)          return AuthDecision::LogAuth;
  return AuthDecision::RejectedAuth;
}
