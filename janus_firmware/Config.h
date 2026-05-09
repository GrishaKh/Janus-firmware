// Config.h — build-time constants only. No runtime state.
// Update pins here when peripherals arrive; runtime state lives elsewhere.
#pragma once

// ----- Firmware version (printed at boot) -----
#define JANUS_FW_VERSION "0.7.0-p6"

// ----- RFID driver selection -----
// Default: MFRC522 over SPI (the blue RC522 board). To experiment with PN532
// later, define RFID_DRIVER_PN532 before including Config.h.
// #define RFID_DRIVER_PN532

// ----- v1 pin map (ESP32-WROOM-32, 38-pin DevKit) -----

// RC522 (MFRC522) over SPI — VSPI defaults. RST is wired (the chip really
// uses it, unlike PN532 where the IRQ pin was optional).
#define PIN_RFID_SCK     18
#define PIN_RFID_MISO    19
#define PIN_RFID_MOSI    23
#define PIN_RFID_SS       5
#define PIN_RFID_RST      4

// DS1302 over 3-wire serial (NOT I2C — the DS1302 has its own bit-banged
// protocol). Use the Makuna `Rtc` library, ThreeWire bus.
#define PIN_RTC_CE       13   // chip-enable / reset (output)
#define PIN_RTC_IO       14   // bidirectional data line
#define PIN_RTC_SCLK     32   // serial clock (output)

// SIM800C V6.1 over UART2 — ESP32 UART2 defaults. The V6.1 carrier has an
// onboard LDO and is friendlier than a raw SIM800L; the 1000 uF cap across
// VCC/GND is still recommended for transmit bursts.
#define PIN_SIM_TX       17   // ESP32 transmits to SIM800C on this pin
#define PIN_SIM_RX       16   // ESP32 receives from SIM800C on this pin
#define PIN_SIM_PWRKEY   25   // reserved; V6.1 typically auto-starts

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
// I2C bus (free in v1; reserved for OLED SSD1306 in v2)
//   #define PIN_I2C_SDA    21
//   #define PIN_I2C_SCL    22

// ----- Intervals (milliseconds) -----
#define HEARTBEAT_INTERVAL_MS    30000UL    // 30 s — /api/attendance/heartbeat
#define CONFIG_POLL_INTERVAL_MS  300000UL   // 5 min — /api/attendance/config
#define RESYNC_INTERVAL_MS       10000UL    // 10 s — drain ring via /resync when non-empty
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
