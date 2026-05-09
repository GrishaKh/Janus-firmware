# Janus-firmware

ESP32 (Arduino) firmware for **Janus**, the RFID + fingerprint entry-control system for the Armath Arapi makerspace. Pairs with the Janus backend at https://github.com/GrishaKh/Janus.

> **Status: v1 in progress.** Bring-up only — empty `setup()` / `loop()` and a green CI. See `docs/` and the Phase plan for what comes next.

## v1 hardware target

- **MCU:** ESP32-WROOM-32 (38-pin DevKit)
- **RFID:** MFRC522 (RC522) over SPI
- **GSM:** SIM800C V6.1 over UART2 (SMS-only, on FreeRTOS task pinned to core 0)
- **RTC:** DS1302 over 3-wire serial (battery-backed)

Deferred to v2 (parts not in hand): R503 fingerprint, E18-D80NK IR beam, SSD1306 OLED, microSD module, active buzzer + IRLZ44N driver, magnetic tamper reed, ideal-diode UPS power.

Pin map and BOM live in [`docs/HARDWARE.md`](docs/HARDWARE.md).

## Quickstart

Prerequisites: `arduino-cli` ≥ 1.0.0 on PATH.

```sh
git clone https://github.com/GrishaKh/Janus-firmware.git
cd Janus-firmware

# 1. Install ESP32 core + pinned libraries
./scripts/lib-install.sh

# 2. Copy the secrets template and fill it in
cp janus_firmware/Secrets.h.example janus_firmware/Secrets.h
$EDITOR janus_firmware/Secrets.h

# 3. Compile
./scripts/build.sh

# 4. Flash + open serial monitor (set PORT first if not /dev/ttyUSB0)
PORT=/dev/ttyUSB0 ./scripts/flash.sh
```

Provisioning a device:

1. Run `pnpm seed` in the Janus backend repo. The script prints two deterministic
   bearer tokens — copy one (e.g. `demo-front-door.demo-token-front-door-7f3a9b1c4e8d2a6b`).
2. Split it on the first `.` and write the two halves into `Secrets.h` as
   `DEVICE_ID` and `RAW_TOKEN`.
3. Set `API_BASE_URL` to your deployed Janus URL (no trailing slash).
4. Flash. See [`docs/PROVISIONING.md`](docs/PROVISIONING.md) for the future BLE / captive-portal path.

## Project layout

```
Janus-firmware/
├── docs/                  HARDWARE, PROTOCOL, PROVISIONING, TROUBLESHOOTING
├── scripts/               build, flash, ci, lib-install
├── arduino-lib-list.txt   pinned library versions (source of truth)
├── .github/workflows/     compile-only CI on push
└── janus_firmware/        the Arduino sketch
    ├── janus_firmware.ino
    ├── Config.h           pins, intervals, FW_VERSION
    ├── Secrets.h.example  template (real Secrets.h is gitignored)
    └── stubs/             empty headers reserved for v2 modules
```

## License

[MIT](LICENSE).
