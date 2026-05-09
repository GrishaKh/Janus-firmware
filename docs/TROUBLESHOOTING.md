# TROUBLESHOOTING

Common bring-up symptoms, in roughly the order you'll hit them. Updated as the build progresses.

## Compile / install

### `arduino-cli: command not found`
Install per https://arduino.github.io/arduino-cli/latest/installation/. On Linux:

```sh
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$HOME/.local/bin sh
```

### `Platform esp32:esp32@3.0.5 not found`
Run `./scripts/lib-install.sh`. It updates the index with the ESP32 board manager URL before installing.

### Library version mismatch
`arduino-lib-list.txt` is the source of truth. CI installs from it; make sure your local toolchain matches:
```sh
./scripts/lib-install.sh
```

### Sketch directory name mismatch
Arduino requires the `.ino` filename to match its containing directory: `janus_firmware/janus_firmware.ino`. Don't rename one without the other.

## Flashing / serial

### `permission denied: /dev/ttyUSB0`
Add yourself to the `dialout` group, log out, log back in:
```sh
sudo usermod -aG dialout $USER
```

### Board not detected
List candidates: `arduino-cli board list`. Common issues:
- Missing CP210x or CH340 USB-serial driver.
- USB cable is power-only (no data lines). Try a different cable.
- Hold the **BOOT** button while plugging in if upload fails to start.

### Garbled serial output
Mismatched baud. The firmware uses **115200**. The flash script enforces this; if you're using a different terminal, set it explicitly.

## Runtime

### Resets / brown-outs once SIM800C is wired (P7+)
Single biggest reliability issue on USB power. Symptoms: ESP32 reboots whenever the GSM module tries to register on the network. Less common on the SIM800C V6.1 board (onboard LDO smooths out current spikes), but still possible on cold-network handshake.

**Fix order:**
1. **1000 µF cap** directly across the GSM module's `VCC`/`GND` on the breadboard.
2. Stronger USB power source — USB power-bank ≥ 2 A, or a 5 V / 2 A wall adapter into a USB-power module.
3. Eventually: dedicated 4 V / 2 A buck rail for the GSM module (v2 power architecture).

### `WiFi connect timeout`
- Verify `WIFI_SSID` and `WIFI_PASS` in `Secrets.h`.
- ESP32 supports 2.4 GHz only — won't see 5 GHz networks.
- Move closer to the AP for bring-up; antenna is on the chip module.

### `NTP failed, falling back to DS3231`
Expected if the LAN blocks outbound UDP/123. The DS3231 fallback runs as long as the coin cell is fresh. After a 12+ hour outage, replace the CR2032.

### `401 — provisioning required`
The backend doesn't recognize the bearer. Check:
1. `DEVICE_ID` and `RAW_TOKEN` exactly match the seed-script output (no whitespace, no swapped halves).
2. The device hasn't been deleted from the admin dashboard.
3. The token wasn't rotated. Re-flash with the current value.

### Events accepted on /events but not visible in admin
- Open the admin dashboard's **live feed** view (subscribes via Supabase Realtime).
- The browser's tab must be focused for some Supabase Realtime versions.
- The seed script's data uses `demo-*` IDs — make sure your filters aren't hiding them.

### Duplicate events / re-posts
Expected and harmless — `event_id` makes /events idempotent. The first call gets 201, subsequent calls return 200 with `status:"duplicate"`. The firmware treats both as "delivered, free the ring slot".
