// SmsSender.h — non-blocking SMS dispatch via SIM800C V6.1.
//
// Owns UART2 exclusively. begin() spawns a FreeRTOS task pinned to core 0
// that initializes the modem lazily (on first enqueue) and drains the
// internal xQueue. enqueue() is fire-and-forget — the main loop never
// blocks on the modem's 10–30 s startup or 5–15 s send latency.
//
// SIM800L vs SIM800C: same AT command set, TinyGSM's
// `TINY_GSM_MODEM_SIM800` define covers both.
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

class SmsSender {
 public:
  void begin();

  // Returns false if the queue is full (caller can choose to drop or
  // retry later). Otherwise the message is queued and the task will
  // attempt to send it.
  bool enqueue(const String& phoneE164, const String& body);

 private:
  // xQueue requires fixed-size POD items.
  struct Item {
    char phone[24];   // E.164 with + and slack
    char body[160];   // single-segment GSM-7 max + slack
  };

  QueueHandle_t _queue = nullptr;
  TaskHandle_t  _task  = nullptr;

  static void taskEntry(void* arg);
  void        taskLoop();
  bool        ensureModemReady();
};
