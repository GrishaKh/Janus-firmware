// Logger.h — abstract event-buffer interface.
//
// Why an interface instead of a concrete class: v1 uses a small RAM ring
// (RamRingLogger). v2 swaps to an SD-backed NDJSON file (SdLogger) for
// real durability across power loss. Both implement this interface so the
// rest of the firmware doesn't care which one is bound.
//
// peek/drop are NOT part of the interface — they're tied to the in-memory
// representation and live on the concrete class. The drainer code in
// the .ino holds a `RamRingLogger&` directly until SdLogger lands.
#pragma once

#include <Arduino.h>

class Logger {
 public:
  virtual ~Logger() = default;

  virtual void   record(const String& eventId, const String& json) = 0;
  virtual size_t pendingCount() const = 0;
};
