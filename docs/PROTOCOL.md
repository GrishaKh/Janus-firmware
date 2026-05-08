# PROTOCOL

Condensed mirror of the four backend endpoints the firmware calls. Source of truth lives in the Janus repo (`app/api/attendance/*`); copy the rules here current as of P0 to make firmware reasoning self-contained.

All requests carry:

```
Authorization: Bearer <device_id>.<raw_token>
Content-Type: application/json
```

The bearer is bcrypt-verified server-side. `device_id` matches `^[a-z0-9][a-z0-9\-_]{2,63}$/i`. `raw_token` is ≥ 16 chars.

## POST /api/attendance/events

Single event. Body shape:

```jsonc
// auth event
{
  "type": "auth",
  "auth_method": "rfid" | "fingerprint" | "2fa",
  "rfid_uid": "04A3B2C1D0",          // one of these three identifiers
  "fingerprint_id": 1,               //   is required (1..1000 for fp)
  "student_code": "ARM-0001",        //
  "occurred_at": "2026-05-07T12:34:56.789Z",  // ISO 8601, parseable by Date.parse
  "event_id": "demo-front-door-1735689600123-42",  // ≤ 64 chars
  "raw_identifier": "..."            // optional, for debugging
}

// breach event
{
  "type": "breach",
  "reason": "no_auth" | "rejected_auth" | "tamper",
  "attempted_source": "rfid" | "fingerprint",  // optional
  "attempted_id": "...",                       // optional
  "occurred_at": "...",
  "event_id": "..."
}
```

Status codes:

| Code | Meaning | Firmware action |
|---|---|---|
| 201 | Logged / breach inserted | Drop from ring buffer. |
| 200 + `{status:"duplicate"}` | Same `event_id` already stored — backend is idempotent | Drop from ring buffer (treat as success). |
| 200 + `{status:"maintenance_dropped"}` | Device is in `maintenance` mode — server discarded | Drop from ring buffer. |
| 400 | Validation failure | Log, drop. Do **not** retry. |
| 401 | Auth failure | Latch `provisioningRequired`, blink 3-pulse LED, suspend new POSTs until reboot. |
| 429 | Rate-limited | Honor `Retry-After` header. |
| 500 / 503 | Server / DB unavailable | Exponential backoff (1, 3, 9, 30 s; cap 4 attempts). Leave in ring buffer. |

## POST /api/attendance/heartbeat

```json
{ "battery_percent": 87 }   // optional, 0..100
```

Response:

```json
{
  "status": "ok",
  "mode": "attendance",
  "alarm_silenced_until": null,
  "silent_from": "19:00:00",
  "silent_to": "07:00:00"
}
```

The device should apply the returned mode to local `DeviceState` so it self-corrects between explicit `/config` polls.

## POST /api/attendance/resync

```json
{ "events": [ /* up to 200 event objects, same shape as /events */ ] }
```

Response:

```json
{
  "summary": {
    "accepted_logs": 5,
    "accepted_breaches": 0,
    "duplicates": 1,
    "invalid": 0,
    "maintenance_dropped": 0,
    "errors": 0
  }
}
```

`accepted_*` and `duplicates` both mean "delivered, free the slot". `invalid` and `errors` count rows the firmware should NOT retry (drop after one attempt).

Firmware uses batches of **50** events per resync call (not the server's 200 max) to keep one HTTPS request under ~30 KB.

## GET /api/attendance/config

Response:

```jsonc
{
  "device_id": "demo-front-door",
  "display_name": "Front Door (Demo)",
  "mode": "attendance",
  "silent_from": "19:00:00",
  "silent_to": "07:00:00",
  "two_factor": false,
  "alarm_silenced_until": null,
  "admin_phones": ["+374..."],
  "enrolled": {
    "rfid_uids": ["04A3B2C1D0", "..."],
    "fingerprint_ids": [1, 2, 3],
    "student_codes": ["ARM-0001", "..."],
    "count": 6
  },
  "server_time": "2026-05-07T12:34:56.789Z"
}
```

`server_time` is the secondary clock-sanity probe — if the device clock is off by > 2 min vs. `server_time`, hard-resync NTP and the DS3231.

## Idempotency rule

Every event the firmware generates carries an `event_id` of the form:

```
<DEVICE_ID>-<unix_ms>-<monotonic_counter>
```

The counter survives reboots via NVS so collisions are impossible. This is what makes retry-after-network-blip safe — the backend is hard-idempotent on `event_id` and returns 200 with the original row's `id` on duplicates.
