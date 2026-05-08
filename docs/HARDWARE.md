# HARDWARE

Pin map, BOM, and wiring notes for Janus-firmware.

## v1 BOM (in hand)

| Component | Notes |
|---|---|
| ESP32-WROOM-32 (38-pin DevKit) | 3.3 V logic, dual-core 240 MHz, 4 MB flash. |
| RFID reader (blue PCB) | Assumed PN532 over SPI. Fallback to MFRC522 via `#define RFID_DRIVER_MFRC522` in `Config.h`. |
| SIM800L | 2.8 V logic on its TX line — voltage-divide before feeding ESP32 RX, or use a level shifter. |
| DS3231 | Battery-backed RTC over I²C, address `0x68`. |

## v1 pin map

| Function | GPIO | Notes |
|---|---|---|
| **PN532 SPI** | SCK=18, MISO=19, MOSI=23, SS=5, IRQ=4 (optional) | Standard VSPI. Set the PN532's DIP switches to SPI mode. |
| **DS3231 I²C** | SDA=21, SCL=22 | Default I²C. The future OLED shares this bus (different address). |
| **SIM800L UART2** | TX (ESP32→SIM)=17, RX (SIM→ESP32)=16 | UART2 default pins. |
| **SIM800L PWRKEY** | GPIO 25 | Reserved; most breakouts auto-start, leave unused. |
| **Built-in LED** | GPIO 2 | Boot/status indicator (steady, blinking, 3-pulse). |

### Reserved for v2

| Function | GPIO | Notes |
|---|---|---|
| Fingerprint UART1 | TX=27, RX=26 | UART1 remap. |
| IR beam input (E18-D80NK) | GPIO 34 | Input-only; external 10 kΩ pull-up (open-collector output). |
| Buzzer MOSFET gate | GPIO 33 | PWM-capable. |
| Tamper reed | GPIO 35 | Input-only; external 10 kΩ pull-up. |
| SD CS | GPIO 15 | Shares VSPI bus with PN532 (different CS). |
| OLED SSD1306 | shares 21/22 with RTC | I²C addresses don't collide (RTC `0x68`, SSD1306 `0x3C`). |

## ⚠ SIM800L brown-outs

The SIM800L pulls **2 A peaks** during transmission and is the #1 cause of resets on USB-powered breadboards.

**Mitigation in v1:**
- Solder a **1000 µF electrolytic cap** directly across SIM800L `Vcc`/`GND` on the breadboard.
- If the host PC's USB port still browns out the ESP32, power the board from a high-current USB power bank or a 5 V / ≥ 2 A wall adapter.

**Long-term (v2):** SIM800L gets its own dedicated 4 V / 2 A buck rail, isolated from the ESP32's 5 V rail. Never share rails. See the reference design at `~/Armath/Arapi/Attendance_System/Armath_Attendance_System.html` for the full three-rail topology with ideal-diode UPS.

## v2 BOM (to acquire)

- **Fingerprint:** R503 capacitive (works through acrylic, 3.3 V native). Optical R307/AS608 is cheaper but doesn't penetrate enclosure material.
- **IR beam:** E18-D80NK adjustable photoelectric switch.
- **Display:** SSD1306 128×64 OLED, I²C.
- **microSD module:** SPI breakout for durable NDJSON event logging.
- **Buzzer:** active 5 V buzzer + IRLZ44N MOSFET driver.
- **Tamper:** magnetic reed switch (NC).
- **Power:** 12 V / 3 A mains adapter, 2× 18650 with TP4056 charger + MT3608 booster, ideal-diode OR (LM66100 or Schottky), three buck/LDO rails.
