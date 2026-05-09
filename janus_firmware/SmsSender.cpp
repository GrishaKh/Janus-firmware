// IMPORTANT: TinyGSM requires the modem family to be selected via #define
// BEFORE TinyGsmClient.h is included. Keeping the define and the include
// confined to this single .cpp avoids accidental include-order bugs.
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 512

#include "SmsSender.h"

#include <HardwareSerial.h>
#include <TinyGsmClient.h>

#include "Config.h"

namespace {
constexpr unsigned long kSimBaud           = 9600;
constexpr unsigned long kAtTimeoutMs       = 30000;   // AT/OK probe budget
constexpr unsigned long kNetTimeoutMs      = 60000;   // CREG registration budget
constexpr UBaseType_t   kQueueDepth        = 8;
constexpr uint32_t      kTaskStackBytes    = 6144;
constexpr UBaseType_t   kTaskPriority      = 1;       // low — WiFi gets priority
constexpr BaseType_t    kTaskCore          = 0;       // pinned to core 0

HardwareSerial   gSimUart(2);  // UART2 (TX=PIN_SIM_TX, RX=PIN_SIM_RX)
TinyGsm          gModem(gSimUart);
bool             gModemReady = false;
}  // namespace

void SmsSender::begin() {
  if (_queue != nullptr) return;  // idempotent

  _queue = xQueueCreate(kQueueDepth, sizeof(Item));
  if (_queue == nullptr) {
    Serial.println(F("[sms] FATAL: xQueueCreate failed"));
    return;
  }

  BaseType_t ok = xTaskCreatePinnedToCore(
      &SmsSender::taskEntry, "sms",
      kTaskStackBytes, this, kTaskPriority, &_task, kTaskCore);
  if (ok != pdPASS) {
    Serial.println(F("[sms] FATAL: xTaskCreatePinnedToCore failed"));
  }
}

bool SmsSender::enqueue(const String& phoneE164, const String& body) {
  if (_queue == nullptr) return false;

  Item item{};
  // strncpy doesn't null-terminate when truncating, so do it manually.
  strncpy(item.phone, phoneE164.c_str(), sizeof(item.phone) - 1);
  strncpy(item.body,  body.c_str(),      sizeof(item.body)  - 1);

  // Don't block the caller — drop if the queue is full. Operator will see
  // it in serial. P9 hardening could promote this to "drop oldest" if the
  // operator wants newer breaches to win.
  if (xQueueSend(_queue, &item, 0) != pdPASS) {
    Serial.println(F("[sms] queue full, dropping message"));
    return false;
  }
  return true;
}

void SmsSender::taskEntry(void* arg) {
  static_cast<SmsSender*>(arg)->taskLoop();
}

void SmsSender::taskLoop() {
  for (;;) {
    Item item;
    if (xQueueReceive(_queue, &item, portMAX_DELAY) != pdPASS) continue;

    if (!ensureModemReady()) {
      Serial.println(F("[sms] modem not ready, dropping message"));
      // Don't requeue — same modem state will fail the same way. P9 could
      // add a "retry on next boot" disk-backed queue if reliability needs it.
      continue;
    }

    Serial.print(F("[sms] sending to "));
    Serial.print(item.phone);
    Serial.print(F(": \""));
    Serial.print(item.body);
    Serial.println(F("\""));

    bool ok = gModem.sendSMS(String(item.phone), String(item.body));
    Serial.println(ok ? F("[sms] sent") : F("[sms] send failed"));
  }
}

bool SmsSender::ensureModemReady() {
  if (gModemReady) return true;

  Serial.println(F("[sms] initializing SIM800C..."));
  gSimUart.begin(kSimBaud, SERIAL_8N1, PIN_SIM_RX, PIN_SIM_TX);
  delay(1500);  // settle time after powering / opening UART

  if (!gModem.testAT(kAtTimeoutMs)) {
    Serial.println(F("[sms] no AT/OK from modem"));
    return false;
  }
  gModem.init();

  String op = gModem.getOperator();
  if (op.length() > 0) {
    Serial.print(F("[sms] operator: "));
    Serial.println(op);
  }

  if (!gModem.waitForNetwork(kNetTimeoutMs)) {
    Serial.println(F("[sms] failed to register on network"));
    return false;
  }
  int rssi = gModem.getSignalQuality();
  Serial.print(F("[sms] modem ready, RSSI="));
  Serial.print(rssi);
  Serial.println(F(" (0..31; <8 means too weak)"));

  // GSM-7 charset; UCS2 path for non-Latin SMS lands in v2.
  gModem.sendAT(GF("+CSCS=\"GSM\""));
  gModem.waitResponse();

  gModemReady = true;
  return true;
}
