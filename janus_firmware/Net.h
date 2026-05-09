// Net.h — owns WiFi, TLS, and the bearer-authed HTTPS client.
// All API calls go through Net::request(); higher modules build paths/bodies.
#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>

class Net {
 public:
  // Result of one HTTPS request.
  struct Response {
    int   status;   // HTTP status code on success, negative on local error
    String body;    // response body (empty on local error)
    bool  ok() const { return status >= 200 && status < 300; }
  };

  // Connect WiFi, prepare TLS context. Blocks until WiFi is up or
  // the connect attempt times out. Idempotent — safe to call again.
  void begin();

  // True iff the WiFi link is currently up.
  bool isOnline();

  // Latched after a 401 from any request — caller should stop POSTing
  // until reboot + reflash with valid creds. Reset only by reboot.
  bool needsProvisioning() const { return _provisioningRequired; }

  // GET <API_BASE_URL><path>. Bearer header is added automatically.
  Response get(const char* path);

  // POST <API_BASE_URL><path> with the given JSON body.
  Response post(const char* path, const String& body);

 private:
  WiFiClientSecure _tls;
  String           _bearer;          // "Bearer <device_id>.<raw_token>"
  bool             _provisioningRequired = false;
  bool             _begun = false;

  // Try once (no retries). Negative status on local errors.
  Response requestOnce(const char* method, const char* path, const String* body);

  // Apply exponential backoff and retry policy on top of requestOnce.
  Response request(const char* method, const char* path, const String* body);

  bool ensureWifi();
};
