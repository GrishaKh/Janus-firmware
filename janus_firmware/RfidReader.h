// RfidReader.h — non-blocking poll wrapper around the MFRC522 (RC522).
//
// Returns each detected card's UID once per "tap" (debounced so a card held
// on the reader doesn't fire repeatedly). UIDs are uppercase hex strings,
// e.g. `04A3B2C1` for a 4-byte Mifare Classic or `04A3B2C1D0E1F2` for a
// 7-byte NTAG. This format matches what the backend seeds and what
// `IdentityCache::contains()` will compare against in P4.
#pragma once

#include <Arduino.h>

class RfidReader {
 public:
  void begin();

  // Non-blocking. Returns true exactly once per card-tap; fills `uid`.
  // Same UID re-read within kDebounceMs is suppressed.
  bool poll(String& uid);

 private:
  static constexpr unsigned long kDebounceMs = 2000;

  String        _lastUid;
  unsigned long _lastReadAtMs = 0;
};
