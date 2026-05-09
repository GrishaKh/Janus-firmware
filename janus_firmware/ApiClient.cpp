#include "ApiClient.h"

#include <ArduinoJson.h>

ConfigResult ApiClient::getConfig() {
  ConfigResult out;

  Net::Response r = _net.get("/api/attendance/config");
  out.status = r.status;

  if (!r.ok()) {
    out.error = String("HTTP ") + r.status;
    if (r.body.length() > 0 && r.body.length() < 200) {
      out.error += " — " + r.body;
    }
    return out;
  }

  // ESP32 has plenty of heap; ArduinoJson v7 grows on demand. The /config
  // body is roughly 60 bytes per enrolled student plus ~300 bytes scalars
  // (so ~12 KB at 200 cards). The default JsonDocument is heap-backed.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, r.body);
  if (err) {
    out.error = String("json: ") + err.c_str();
    return out;
  }

  out.device_id           = doc["device_id"]    | "";
  out.display_name        = doc["display_name"] | "";
  out.mode                = doc["mode"]         | "attendance";
  out.two_factor          = doc["two_factor"]   | false;
  out.silent_from         = doc["silent_from"]  | "";
  out.silent_to           = doc["silent_to"]    | "";
  out.alarm_silenced_until= doc["alarm_silenced_until"] | "";
  out.server_time         = doc["server_time"]  | "";
  out.enrolled_count      = doc["enrolled"]["count"] | 0;

  JsonArrayConst uidArr = doc["enrolled"]["rfid_uids"];
  out.rfid_uids.reserve(uidArr.size());
  for (JsonVariantConst v : uidArr) {
    const char* s = v.as<const char*>();
    if (s != nullptr) out.rfid_uids.push_back(String(s));
  }

  JsonArrayConst phoneArr = doc["admin_phones"];
  out.admin_phones.reserve(phoneArr.size());
  for (JsonVariantConst v : phoneArr) {
    const char* s = v.as<const char*>();
    if (s != nullptr && s[0] != '\0') out.admin_phones.push_back(String(s));
  }

  out.ok = true;
  return out;
}

PostEventResult ApiClient::postEvent(const String& jsonBody) {
  PostEventResult out;
  Net::Response r = _net.post("/api/attendance/events", jsonBody);
  out.status = r.status;

  if (r.status == 201) {
    out.ok = true;
    return out;
  }
  if (r.status == 200) {
    // Backend returns 200 for two idempotent / mode-related cases:
    //   {"status":"duplicate", ...} — already stored, free the ring slot
    //   {"status":"maintenance_dropped"} — server suppressed by mode
    out.ok = true;
    if (r.body.indexOf("\"duplicate\"") >= 0)            out.duplicate = true;
    if (r.body.indexOf("\"maintenance_dropped\"") >= 0)  out.maintenance_dropped = true;
    return out;
  }

  out.error = String("HTTP ") + r.status;
  if (r.body.length() > 0 && r.body.length() < 200) {
    out.error += " — " + r.body;
  }
  return out;
}

ResyncResult ApiClient::resync(const String& wrappedBody) {
  ResyncResult out;
  Net::Response r = _net.post("/api/attendance/resync", wrappedBody);
  out.status = r.status;

  if (r.status != 200) {
    out.error = String("HTTP ") + r.status;
    if (r.body.length() > 0 && r.body.length() < 200) {
      out.error += " — " + r.body;
    }
    return out;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, r.body);
  if (err) {
    out.error = String("json: ") + err.c_str();
    return out;
  }

  JsonObjectConst s = doc["summary"];
  out.accepted             = (int)(s["accepted_logs"]      | 0)
                           + (int)(s["accepted_breaches"]  | 0);
  out.duplicates           = (int)(s["duplicates"]         | 0);
  out.maintenance_dropped  = (int)(s["maintenance_dropped"]| 0);
  out.invalid              = (int)(s["invalid"]            | 0);
  out.errors               = (int)(s["errors"]             | 0);
  out.ok = true;
  return out;
}
