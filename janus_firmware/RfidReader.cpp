#include "RfidReader.h"

#include <MFRC522.h>
#include <SPI.h>

#include "Config.h"

namespace {
MFRC522 gMfrc(PIN_RFID_SS, PIN_RFID_RST);

String toUpperHex(const byte* bytes, byte len) {
  String s;
  s.reserve(len * 2);
  for (byte i = 0; i < len; i++) {
    if (bytes[i] < 0x10) s += '0';
    s += String(bytes[i], HEX);
  }
  s.toUpperCase();
  return s;
}
}  // namespace

void RfidReader::begin() {
  SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
  gMfrc.PCD_Init();
  // Max antenna gain — default 32 dB is conservative; 48 dB is reliable
  // through plastic enclosures and at the typical 2–3 cm read range.
  gMfrc.PCD_SetAntennaGain(MFRC522::RxGain_max);

  byte v = gMfrc.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("[rfid] MFRC522 version 0x"));
  Serial.println(v, HEX);
  if (v == 0x00 || v == 0xFF) {
    Serial.println(F("[rfid] WARNING: chip not responding "
                     "(check SPI wiring + RST + 3.3V power)"));
  }
}

bool RfidReader::poll(String& uid) {
  if (!gMfrc.PICC_IsNewCardPresent())  return false;
  if (!gMfrc.PICC_ReadCardSerial())    return false;

  String formatted = toUpperHex(gMfrc.uid.uidByte, gMfrc.uid.size);
  gMfrc.PICC_HaltA();

  unsigned long now = millis();
  if (formatted == _lastUid && (now - _lastReadAtMs) < kDebounceMs) {
    return false;  // same card held on reader, suppress duplicate
  }
  _lastUid = formatted;
  _lastReadAtMs = now;
  uid = formatted;
  return true;
}
