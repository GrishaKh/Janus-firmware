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

  out.ok = true;
  return out;
}
