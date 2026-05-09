#include "TimeKeeper.h"

#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <WiFi.h>
#include <sys/time.h>
#include <time.h>

#include "Config.h"

namespace {
// Order is (IO, SCLK, CE) per Makuna's API.
ThreeWire             gWire(PIN_RTC_IO, PIN_RTC_SCLK, PIN_RTC_CE);
RtcDS1302<ThreeWire>  gRtc(gWire);

// 2025-01-01T00:00:00Z. Anything before this means time is uninitialized.
constexpr time_t kEpoch2025 = 1735689600;

// Convert an ISO-8601-Z string ("YYYY-MM-DDTHH:MM:SS[.mmm]Z") into UTC
// seconds since 1970-01-01. Returns 0 on parse failure.
time_t parseIsoUtc(const String& iso) {
  if (iso.length() < 19) return 0;
  int y, mo, d, h, mi, s;
  if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) != 6) return 0;
  if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0;

  struct tm gm = {};
  gm.tm_year  = y - 1900;
  gm.tm_mon   = mo - 1;
  gm.tm_mday  = d;
  gm.tm_hour  = h;
  gm.tm_min   = mi;
  gm.tm_sec   = s;
  gm.tm_isdst = 0;
  // ESP-IDF newlib doesn't ship timegm. We set TZ=UTC0 in TimeKeeper::begin
  // before any caller can land here, so mktime interprets the struct as UTC.
  return mktime(&gm);
}
}  // namespace

void TimeKeeper::begin() {
  // Tell the C runtime we want UTC end-to-end. configTime() also does this,
  // but we want it set before the RTC fallback path runs too.
  setenv("TZ", "UTC0", 1);
  tzset();

  gRtc.Begin();
  if (gRtc.GetIsWriteProtected()) {
    gRtc.SetIsWriteProtected(false);
  }
  if (!gRtc.GetIsRunning()) {
    Serial.println(F("[time] DS1302 was halted, kicking the oscillator"));
    gRtc.SetIsRunning(true);
  }
  _rtcOk = gRtc.IsDateTimeValid();
  if (!_rtcOk) {
    Serial.println(F("[time] DS1302 reports invalid datetime "
                     "(battery dead or never set)"));
  }

  if (WiFi.status() == WL_CONNECTED && tryNtp(NTP_INITIAL_TIMEOUT_MS)) {
    _ntpSucceeded = true;
    _lastTrustedSyncMs = millis();
    syncRtcFromSystem();
    Serial.print(F("[time] NTP synced: "));
    Serial.println(nowIso8601());
  } else if (_rtcOk) {
    syncSystemFromRtc();
    _lastTrustedSyncMs = millis();
    Serial.print(F("[time] DS1302 fallback: "));
    Serial.println(nowIso8601());
  } else {
    Serial.println(F("[time] WARNING: no time source available — events will defer"));
  }
}

String TimeKeeper::nowIso8601() {
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) != 0) return String();
  if (tv.tv_sec < kEpoch2025) return String();

  struct tm gm;
  gmtime_r(&tv.tv_sec, &gm);

  char buf[32];
  size_t n = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &gm);
  snprintf(buf + n, sizeof(buf) - n, ".%03ldZ", (long)(tv.tv_usec / 1000));
  return String(buf);
}

bool TimeKeeper::isTimeSane() {
  if (_lastTrustedSyncMs == 0) return false;
  if (millis() - _lastTrustedSyncMs > TIME_DRIFT_MAX_MS) return false;
  return time(nullptr) > kEpoch2025;
}

void TimeKeeper::noteServerTime(const String& iso) {
  time_t serverT = parseIsoUtc(iso);
  if (serverT <= 0) {
    Serial.print(F("[time] could not parse server_time: "));
    Serial.println(iso);
    return;
  }
  time_t localT = time(nullptr);
  long diffSec = (long)(serverT - localT);

  if (labs(diffSec) > (long)(TIME_RESYNC_THRESHOLD_MS / 1000)) {
    Serial.print(F("[time] hard resync from server (drift "));
    Serial.print(diffSec);
    Serial.println(F("s)"));
    struct timeval tv = { .tv_sec = serverT, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
    syncRtcFromSystem();
    _lastTrustedSyncMs = millis();
  } else {
    // Within tolerance — refresh the trusted-sync window so the drift gate
    // stays open even if NTP died right after boot.
    _lastTrustedSyncMs = millis();
  }
}

bool TimeKeeper::tryNtp(unsigned long timeout_ms) {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  unsigned long deadline = millis() + timeout_ms;
  while (time(nullptr) < kEpoch2025 && (long)(deadline - millis()) > 0) {
    delay(200);
  }
  return time(nullptr) >= kEpoch2025;
}

void TimeKeeper::syncSystemFromRtc() {
  if (!gRtc.IsDateTimeValid()) return;
  RtcDateTime r = gRtc.GetDateTime();
  // Unix32Time() returns seconds since 1970-01-01, UTC (Makuna 2.5+).
  time_t t = r.Unix32Time();
  if (t < kEpoch2025) return;
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
}

void TimeKeeper::syncRtcFromSystem() {
  time_t t = time(nullptr);
  if (t < kEpoch2025) return;
  RtcDateTime r;
  r.InitWithUnix32Time(t);
  gRtc.SetDateTime(r);
}
