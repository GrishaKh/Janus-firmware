# PROVISIONING

How a Janus device gets the credentials it needs to talk to the backend.

## v1 (current): hardcoded `Secrets.h`

Workflow:

1. **Mint a device in the admin dashboard.** Either via the UI (`/admin` → Devices → New) or, for the demo, run `pnpm seed` in the Janus backend repo. The seed prints two deterministic bearer tokens that survive re-runs.
2. **Copy the bearer.** It looks like `demo-front-door.demo-token-front-door-7f3a9b1c4e8d2a6b`.
3. **Split on the first `.`** — left side goes to `DEVICE_ID`, right side to `RAW_TOKEN`.
4. **Fill `janus_firmware/Secrets.h`** (copy from `Secrets.h.example`).
5. **Flash:** `./scripts/flash.sh`.

`Secrets.h` is in `.gitignore`. Never commit it.

If the token is rotated server-side, the device gets `401`, latches a "provisioning required" state, and stops POSTing. Re-flash with the new token and reboot.

## v2: BLE / captive-portal provisioning (deferred)

Goal: a freshly-flashed device with no compiled-in credentials can be paired with the backend in the field, without a laptop.

Two viable paths:

### Option A — captive portal (WiFiManager-style)

1. Device boots into AP mode (`Janus-Setup-XXXX`) with no creds.
2. Operator joins from a phone, captive portal prompts for: WiFi SSID + password, API base URL, and a one-time **enrollment token** (issued by the admin dashboard, valid for 10 minutes).
3. Device hits `POST /api/admin/attendance/enroll-device` with the enrollment token; backend mints a fresh device record + raw_token, returns it once.
4. Device persists creds to NVS, reboots into normal mode.
5. Backend marks the enrollment token consumed.

Library candidates: `WiFiManager` by tzapu (well-maintained, async support).

### Option B — BLE companion app

Same flow but a small Android/web-Bluetooth app handles the handshake. More polished UX, more work to build.

### Either way — backend changes needed

A new endpoint `POST /api/admin/attendance/enroll-device` that:
- Accepts an `enrollment_token` issued by the admin (single-use, short TTL, signed).
- Creates an `attendance_devices` row with a fresh bcrypt hash.
- Returns the raw bearer **once** in the response body (then it's gone forever).

## Token rotation

For v1, rotation = re-flash. For v2, the device should:

- On `401`, refuse to keep retrying the same token.
- Optionally, attempt a `POST /api/admin/attendance/rotate-device-token` with a refresh credential (TBD) to self-heal.
- Otherwise, fall back into the provisioning portal.

This is intentionally out of scope until v2 hardware is in hand.
