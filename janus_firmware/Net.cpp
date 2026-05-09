#include "Net.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "Config.h"
#include "Secrets.h"

// ----------------------------------------------------------------------------
// TLS root bundle.
// Vercel terminates TLS with Google Trust Services certs. The leaf chains
// through the WR1 issuing CA to the GTS Root R1 self-signed root. Because
// Vercel does NOT send the intermediate in the handshake, we must pin BOTH
// here so mbedtls can build the chain offline.
//
// Validity windows:
//   WR1            2023-12-13 .. 2029-02-20
//   GTS Root R1    2016-06-22 .. 2036-06-22
//
// If TLS handshakes start failing in 2029, refresh WR1 from
// http://i.pki.goog/wr1.crt (DER → PEM with `openssl x509 -inform der`).
// ----------------------------------------------------------------------------
static const char kRootCaBundlePem[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIFCzCCAvOgAwIBAgIQf9niwtIEigR0tieibQhopzANBgkqhkiG9w0BAQsFADBH
MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM
QzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMjMxMjEzMDkwMDAwWhcNMjkwMjIw
MTQwMDAwWjA7MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNl
cnZpY2VzMQwwCgYDVQQDEwNXUjEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQDPbjYWircr7kaYAx1TcA937qNLoHK+jyMtwkfGj1yN+T3mGo7uMyINyRFI
uLBizvRpDXICfd7VJg/DbpvPfg7XIM/GkDujggbaOp3/bFa/3OlhlEXkabxPD8kT
wK1hRHIggdAPK55oamJqj4oiV3lpK+IkM352YyxdvFFpfiMHsf92gfHuuFi1azUV
76HmSCg5lzHZBx+Vp56uz5i8no2KA+Gwl01Qb5NMSh/4233xkJkVf+OW7e4xgepy
PVId3yVkpQtwqp7oqLlHyKdaECVgb0Lh1z/njwzwwoNGMyDmS3cEdqFop10VGO/Y
KHc1rQ6tRuRibuKq+MzvN34PJrMHAgMBAAGjgf4wgfswDgYDVR0PAQH/BAQDAgGG
MB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/
AgEAMB0GA1UdDgQWBBRmaUnU3iqckQPPiQ4kuA4wA26ILjAfBgNVHSMEGDAWgBTk
rysmcRorSCeFL1JmLO/wiRNxPjA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAKG
GGh0dHA6Ly9pLnBraS5nb29nL3IxLmNydDArBgNVHR8EJDAiMCCgHqAchhpodHRw
Oi8vYy5wa2kuZ29vZy9yL3IxLmNybDATBgNVHSAEDDAKMAgGBmeBDAECATANBgkq
hkiG9w0BAQsFAAOCAgEATuazCBEgkWAn+VGQTQIY7rjBidUihJfm1t/mTjo7KQR+
3iDx4o2L06oeF0Q3wpKYpQgI/TeMqUlYMWQmZbWPE0PX8pfsVAE5E5tVOjh34bNA
JwDPVnsZVJwzN3nw5BGQ7sxRspFzIcM/qbbTpNeXf9II4Wsk2+Tv6FSVFZUL3/0u
HradbruDWjRQ4IZ7mYqKiEqk08dpOZ+TmBzwykEGy1/IXberb6Ap1SSnn2+RI7t6
N/fqPCrwwFjp8kg1G6etRATGBaPYCx+GjJMFPX+k97Alvoj3/98SvqdegLPYEPjv
xUclHpiKLD63NMmVarVQddIL6kOvTe5k0pnxRnR+mndGHIQc77TLbcZFeja56Pyn
lSqmer578c7CBrPqo1BVmPyWUK+v6sGuzs7Mq7QQaxVs4710cI/MpPp1ovxMVt17
ENKxLk34LpEKAKVmqwnzbHHRjhXNeCC984XDOwLEp0K4MzHl8ZOWJQAakCdVlFC+
PyA3GP2JX/QLoqWNHGuN9c9vLObDhHVs/L+65De+OdnnjpFGI9xxtsNyRsyaHdFA
f5z7ulOoXDXkHCCej/Ehs5docReNt16W2xbH/EBuirJrOzFE2rtALxksl1TdEjOf
IKXOJfUqQeVI5+hA7V+n1+A/n7Npg0S+5ODytWh5XW54ccN1drJnMK54ttozh0c=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
)PEM";

// ----------------------------------------------------------------------------

namespace {
constexpr unsigned long kBackoffsMs[] = {1000, 3000, 9000, 30000};
constexpr size_t kMaxAttempts = sizeof(kBackoffsMs) / sizeof(kBackoffsMs[0]);

constexpr unsigned long kWifiConnectTimeoutMs = 20000;

bool isRetryable(int status) {
  if (status < 0) return true;             // local DNS/TCP failure
  if (status == 429) return true;          // honored separately, but retry
  if (status >= 500 && status < 600) return true;
  return false;
}
}  // namespace

void Net::begin() {
  if (_begun) return;
  _begun = true;

  _bearer = String("Bearer ") + DEVICE_ID + "." + RAW_TOKEN;

#ifdef DEV_TLS_INSECURE
  Serial.println(F("[net] WARNING: TLS verification disabled (DEV_TLS_INSECURE)"));
  _tls.setInsecure();
#else
  _tls.setCACert(kRootCaBundlePem);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  ensureWifi();
}

bool Net::isOnline() {
  return WiFi.status() == WL_CONNECTED;
}

bool Net::ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.print(F("[net] WiFi connecting to "));
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const unsigned long deadline = millis() + kWifiConnectTimeoutMs;
  while (WiFi.status() != WL_CONNECTED && (long)(deadline - millis()) > 0) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[net] WiFi connect timeout"));
    return false;
  }

  Serial.print(F("[net] WiFi up: "));
  Serial.print(WiFi.localIP());
  Serial.print(F("  RSSI="));
  Serial.println(WiFi.RSSI());
  return true;
}

Net::Response Net::requestOnce(const char* method, const char* path, const String* body) {
  Response r{-1, String()};

  if (!ensureWifi()) {
    return r;
  }

  String url = String(API_BASE_URL) + path;

  HTTPClient http;
  // The HTTPClient on ESP32 chooses HTTPS automatically when given a
  // WiFiClientSecure and an https:// URL. Use 10s timeout — Vercel cold
  // starts can take a few seconds.
  http.setTimeout(10000);
  http.setConnectTimeout(8000);
  if (!http.begin(_tls, url)) {
    Serial.println(F("[net] http.begin failed"));
    return r;
  }
  http.addHeader("Authorization", _bearer);
  http.addHeader("Accept", "application/json");
  if (body != nullptr) {
    http.addHeader("Content-Type", "application/json");
  }

  int status;
  if (body != nullptr) {
    status = http.sendRequest(method, *body);
  } else {
    status = http.sendRequest(method);
  }

  r.status = status;
  if (status > 0) {
    r.body = http.getString();
  } else {
    Serial.print(F("[net] transport error: "));
    Serial.println(HTTPClient::errorToString(status));
  }
  http.end();
  return r;
}

Net::Response Net::request(const char* method, const char* path, const String* body) {
  if (_provisioningRequired) {
    Serial.println(F("[net] provisioningRequired latched — refusing request"));
    return Response{401, String("{\"error\":\"provisioning_required\"}")};
  }

  for (size_t attempt = 0; attempt < kMaxAttempts; ++attempt) {
    Response r = requestOnce(method, path, body);

    if (r.status == 401) {
      Serial.println(F("[net] 401 — provisioning required; suspending POSTs"));
      _provisioningRequired = true;
      return r;
    }
    if (r.status >= 200 && r.status < 400) {
      return r;
    }
    if (r.status >= 400 && r.status < 500 && r.status != 429) {
      // Validation / not-found etc. — don't retry.
      return r;
    }
    if (!isRetryable(r.status)) {
      return r;
    }

    if (attempt + 1 < kMaxAttempts) {
      unsigned long sleep_ms = kBackoffsMs[attempt];
      Serial.print(F("[net] retrying in "));
      Serial.print(sleep_ms);
      Serial.print(F("ms (attempt "));
      Serial.print(attempt + 2);
      Serial.print('/');
      Serial.print(kMaxAttempts);
      Serial.println(F(")"));
      delay(sleep_ms);
    }
  }

  Serial.println(F("[net] giving up after max attempts"));
  return Response{-1, String()};
}

Net::Response Net::get(const char* path) {
  return request("GET", path, nullptr);
}

Net::Response Net::post(const char* path, const String& body) {
  return request("POST", path, &body);
}
