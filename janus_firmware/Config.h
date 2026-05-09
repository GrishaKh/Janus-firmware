// Config.h — build-time constants only. No runtime state.
// Update pins here when peripherals arrive; runtime state lives elsewhere.
#pragma once

// ----- Firmware version (printed at boot) -----
#define JANUS_FW_VERSION "0.2.0-p1"

// ----- RFID driver selection -----
// Default: PN532 over SPI (matches the blue Adafruit-style board).
// To switch to MFRC522, define this before including Config.h, or uncomment:
// #define RFID_DRIVER_MFRC522

// ----- v1 pin map (ESP32-WROOM-32, 38-pin DevKit) -----

// PN532 over SPI (VSPI defaults)
#define PIN_RFID_SCK     18
#define PIN_RFID_MISO    19
#define PIN_RFID_MOSI    23
#define PIN_RFID_SS       5
#define PIN_RFID_IRQ      4   // optional, leaves wire free if unused

// DS3231 over I2C (default ESP32 I2C)
#define PIN_I2C_SDA      21
#define PIN_I2C_SCL      22

// SIM800L over UART2 (ESP32 UART2 defaults)
#define PIN_SIM_TX       17   // ESP32 transmits to SIM800L on this pin
#define PIN_SIM_RX       16   // ESP32 receives from SIM800L on this pin (use level shift)
#define PIN_SIM_PWRKEY   25   // reserved; many breakouts auto-start

// Built-in status LED on most ESP32 DevKits
#define PIN_STATUS_LED    2

// ----- Reserved for v2 (do not wire yet, but document) -----
// Fingerprint UART1
//   #define PIN_FP_TX      27
//   #define PIN_FP_RX      26
// IR beam input  (E18-D80NK, open-collector, external 10k pull-up)
//   #define PIN_IR_BEAM    34
// Buzzer MOSFET gate
//   #define PIN_BUZZER     33
// Tamper reed (input-only, external 10k pull-up)
//   #define PIN_TAMPER     35
// SD card CS (shares VSPI with RFID)
//   #define PIN_SD_CS      15

// ----- Intervals (milliseconds) -----
#define HEARTBEAT_INTERVAL_MS    30000UL    // 30 s — /api/attendance/heartbeat
#define CONFIG_POLL_INTERVAL_MS  300000UL   // 5 min — /api/attendance/config
#define WIFI_RECONNECT_GRACE_MS  5000UL     // tolerate brief drops before reconnecting
#define NTP_INITIAL_TIMEOUT_MS   10000UL    // give NTP this long at boot before falling back

// ----- Buffers -----
#define EVENT_RING_CAPACITY      64         // ~16 KB at ~250 B per JSON event
#define RESYNC_BATCH_SIZE        50         // /resync rows per HTTPS request

// ----- Time validity gate -----
#define TIME_DRIFT_MAX_MS        (60UL * 60UL * 1000UL)   // ±1 hour from last trusted sync
#define TIME_RESYNC_THRESHOLD_MS (2UL * 60UL * 1000UL)    // > 2 min off vs server_time → re-sync

// ----- Serial -----
#define SERIAL_BAUD             115200
