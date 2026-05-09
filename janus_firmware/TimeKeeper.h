// TimeKeeper.h — UTC clock for the device.
//
// Boot order:
//   1. Bring up the DS1302 over its 3-wire bus.
//   2. If WiFi is up, configure SNTP and wait briefly for the first epoch.
//   3. On NTP success, write the time back to the DS1302 so we survive
//      future boots without WiFi.
//   4. On NTP failure but a valid DS1302 reading, seed the system clock
//      from the RTC.
//
// `nowIso8601()` returns the canonical timestamp Janus events carry, e.g.
// "2026-05-09T12:34:56.789Z". `isTimeSane()` is the drift gate every event
// must pass before being POSTed.
#pragma once

#include <Arduino.h>

class TimeKeeper {
 public:
  void begin();

  String nowIso8601();
  bool   isTimeSane();

  // Called with /config.server_time after every successful /config poll.
  // If the device clock disagrees with the server by more than the
  // configured threshold, hard-resync from the server value.
  void noteServerTime(const String& iso);

  bool rtcAvailable()   const { return _rtcOk; }
  bool ntpSucceeded()   const { return _ntpSucceeded; }

 private:
  bool          _rtcOk = false;
  bool          _ntpSucceeded = false;
  unsigned long _lastTrustedSyncMs = 0;

  bool tryNtp(unsigned long timeout_ms);
  void syncSystemFromRtc();
  void syncRtcFromSystem();
};
