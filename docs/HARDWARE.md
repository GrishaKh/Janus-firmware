# HARDWARE

Pin map, BOM, and wiring notes for Janus-firmware.

## v1 BOM (in hand)

| Component | Notes |
|---|---|
| ESP32-WROOM-32 (38-pin DevKit) | 3.3 V logic, dual-core 240 MHz, 4 MB flash. |
| **MFRC522 (RC522)** RFID reader | Blue PCB, SPI interface, MIFARE / NTAG cards at 13.56 MHz. Library: `MFRC522` by miguelbalboa. |
| **SIM800C V6.1** GSM/GPRS module | Same AT command set as SIM800L. The V6.1 carrier has an onboard LDO and level shifters, much friendlier on USB power than a raw SIM800L. |
| **DS1302** RTC | Three-wire serial protocol (NOT I²C). Library: `Rtc by Makuna`, `ThreeWire` bus. Less accurate than the DS3231 (~5 ppm vs 2 ppm); refresh from server time periodically. |

## v1 pin map

| Function | GPIO | Notes |
|---|---|---|
| **MFRC522 SPI** | SCK=18, MISO=19, MOSI=23, SS=5, RST=4 | Standard VSPI. The RST pin is required by the MFRC522 chip (unlike PN532). |
| **DS1302 ThreeWire** | CE=13, IO=14, SCLK=32 | Bit-banged 3-wire protocol; pick any 3 free GPIOs, not part of any peripheral bus. |
| **SIM800C UART2** | TX (ESP32→SIM)=17, RX (SIM→ESP32)=16 | UART2 default. V6.1 board is 5 V tolerant on its UART, but if you ever sub in a raw SIM800L, voltage-divide the SIM-TX line. |
| **SIM800C PWRKEY** | GPIO 25 | Reserved; V6.1 modules typically auto-start. |
| **Built-in LED** | GPIO 2 | Boot/status indicator (steady, blinking, 3-pulse). |

### Reserved for v2

| Function | GPIO | Notes |
|---|---|---|
| Fingerprint UART1 | TX=27, RX=26 | UART1 remap. |
| IR beam input (E18-D80NK) | GPIO 34 | Input-only; external 10 kΩ pull-up (open-collector output). |
| Buzzer MOSFET gate | GPIO 33 | PWM-capable. |
| Tamper reed | GPIO 35 | Input-only; external 10 kΩ pull-up. |
| SD CS | GPIO 15 | Shares VSPI bus with MFRC522 (different CS). |
| I²C bus (OLED) | SDA=21, SCL=22 | Free in v1 — reserved for the future SSD1306. The DS1302 does *not* use this bus. |

## ⚠ SIM800C / SIM800L brown-outs

GSM TX peaks pull ~1–2 A on either module. The SIM800C V6.1 carrier has onboard regulation that helps, but USB power can still brown out on cold-network registration.

**Mitigation in v1:**
- Solder a **1000 µF electrolytic cap** directly across the SIM800C `VCC`/`GND` pins on the breadboard.
- If the host PC's USB port still resets the ESP32, power the board from a high-current USB power bank or a 5 V / ≥ 2 A wall adapter.

**Long-term (v2):** the GSM module gets its own dedicated 4 V / 2 A buck rail, isolated from the ESP32's 5 V rail.

## ⚠ DS1302 accuracy + battery

The DS1302 is a much older, less accurate RTC than the DS3231 (no temperature compensation). Two implications for v1 firmware:

- **Refresh from `server_time` aggressively.** P2's TimeKeeper still pulls NTP at boot and writes back to the DS1302, but the > 2 min drift gate against `/config.server_time` is what keeps long-running devices on time. Don't rely on the DS1302 for multi-day accuracy.
- **The CR2032 holdover battery doesn't trickle-charge.** Plain DS1302 modules use a non-rechargeable cell — replace it once a year or whenever the firmware logs a "lostPower" warning at boot.

Some DS1302 breakouts ship with a LIR2032 instead and a charge resistor — those *do* trickle-charge from the supply rail. Check your specific board.

## v2 BOM (to acquire)

- **Fingerprint:** R503 capacitive (works through acrylic, 3.3 V native). Optical R307/AS608 is cheaper but doesn't penetrate enclosure material.
- **IR beam:** E18-D80NK adjustable photoelectric switch.
- **Display:** SSD1306 128×64 OLED, I²C — uses the bus that's free in v1 (SDA=21, SCL=22).
- **microSD module:** SPI breakout for durable NDJSON event logging.
- **Buzzer:** active 5 V buzzer + IRLZ44N MOSFET driver.
- **Tamper:** magnetic reed switch (NC).
- **Power:** 12 V / 3 A mains adapter, 2× 18650 with TP4056 charger + MT3608 booster, ideal-diode OR (LM66100 or Schottky), three buck/LDO rails.
- **Optional RTC upgrade:** swap DS1302 → DS3231 once an I²C peripheral is being added anyway. Better accuracy, temp-compensated, fewer GPIOs (just SDA/SCL).
