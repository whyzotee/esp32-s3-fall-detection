# MQTT payload

Default subscription: `tracker/+/location`. The JSON `deviceId` is authoritative; the topic ID is checked when present.

```json
{
  "deviceId": "ABC001",
  "latitude": 14.123,
  "longitude": 100.456,
  "altitude": 35.5,
  "batteryPercent": 82,
  "rssi": -75,
  "snr": 8.5,
  "status": "NORMAL_UPDATE",
  "deviceTime": "12:30:15:20",
  "timestamp": "2026-08-29T12:30:15.200+07:00"
}
```

Statuses: `NORMAL_UPDATE`, `BUTTON_PRESSED`, `FALL_DETECTED`, `LOW_BATTERY`. Invalid JSON, GPS, battery, radio values, status and topic mismatch are rejected into diagnostics and do not crash ingestion.

## SOS and fall episodes

`BUTTON_PRESSED` opens an `SOS` alert and `FALL_DETECTED` opens a `FALL_DETECTED` alert immediately. The episode state is stored in PostgreSQL per Tracker assignment and alert type, so one Tracker never suppresses another Tracker's alert and SOS never suppresses FALL.

Repeated packets of the same active urgent status are still saved as device events and tracking positions. Inside the fixed cooldown window they do not create another alert or another `alert.opened` broadcast, but they also do not postpone the window: the first packet at or after the boundary opens a new alert and starts the next window. The window is controlled from **System status > Alert thresholds > SOS/Fall repeat cooldown** (1–60 minutes, 2 minutes by default). Until an ADMIN saves an override, the default is derived from `URGENT_ALERT_COOLDOWN_SECONDS`. A different status, including `NORMAL_UPDATE`, closes the active episode; a later SOS or fall then alerts immediately.

Set `MQTT_MODE=local`, `external` or `dual`. Dual mode uses event fingerprints to deduplicate the same packet. Credentials are never hard-coded in application source; Mosquitto password and ACL files are generated into a named volume from environment values.

## ChirpStack uplinks

The ChirpStack adapter accepts the decoded JSON uplink demonstrated in RealTest.
Subscribe to `application/APPLICATION_ID/device/+/event/up`, or replace `+` with a
single DevEUI. Application IDs are UUIDs, not AppEUI/JoinEUI values. MQTT topic
matching is case-sensitive. Use the configured filter if the server customizes
its topic format; standard application topics are checked against the envelope.

Set `CHIRPSTACK_ENABLED=true` and `CHIRPSTACK_MQTT_TOPIC` to the desired filter to
receive ChirpStack alongside the existing custom subscription. Alternatively,
set `MQTT_PAYLOAD_MODE=chirpstack` and `MQTT_TOPIC` to that filter for a dedicated
subscription. `CHIRPSTACK_PAYLOAD_MODE` must be `chirpstack`.

## ChirpStack Low Power downlink

Set `CHIRPSTACK_DOWNLINK_TOPIC` to the exact template
`application/APPLICATION_ID/device/DEV_EUI/command/down`. Keep `DEV_EUI` literal;
the backend replaces it with the selected Tracker's lowercase registered DevEUI.
The template is validated at startup. When `MQTT_MODE=local` or `dual`, commands
publish through the local connection; `external` publishes through the external
connection. The publish uses QoS 1, `retain=false` and a five-second timeout.

Low Power ON uses:

```json
{
  "devEui": "70b3d57ed8005343",
  "confirmed": false,
  "fPort": 10,
  "data": "AQAP"
}
```

Low Power OFF uses:

```json
{
  "devEui": "70b3d57ed8005343",
  "confirmed": false,
  "fPort": 10,
  "data": "AAAA"
}
```

`AQAP` decodes to `01 00 0F` and enables Low Power Mode. `AAAA` decodes to
`00 00 00` and disables it. Firmware requires the exact three-byte payload on
FPort 10. A successful API response means the broker accepted the queued publish;
it is not a hardware acknowledgement and Safe Track does not set a low-power
state on the Tracker record.

| Input | Safe Track behavior |
| --- | --- |
| `deviceInfo.devEui` | Matches the registered Tracker's DevEUI, ignoring hex letter case; never falls back to Device ID |
| `deviceInfo.applicationId` | Application identity and topic validation |
| `object.latitude`, `object.longitude` | Tracker position; gateway location is never substituted |
| `object.status` | 0 = normal, 1 = button/SOS, 2 = suspected fall |
| `time` | Event timestamp; fractional seconds accepted, stored at JavaScript millisecond precision |
| `object.time_string` | Device diagnostic time; in debug mode it is elapsed boot time, not UTC |
| `fPort` | Must be 2 for this tracker payload |
| `fCnt` | Unsigned 32-bit uplink counter; gaps and resets are accepted |
| `deduplicationId` | Preferred duplicate identity, scoped to application and DevEUI |
| `rxInfo` | RSSI/SNR from the valid receiver with highest SNR; first receiver wins ties |
| Battery / altitude | Not supplied by this payload; stored as null |

The full envelope is retained in PostgreSQL device events. Missing `object`, bad
GPS/status/time/identity, a wrong FPort or conflicting standard topic is rejected
with a diagnostic reason. Binary gateway events are not application JSON. The
adapter does not decode Base64 `data`; configure the device-profile codec in
ChirpStack. Invalid optional receiver measurements are ignored.

If `deduplicationId` is absent, deduplication uses application, DevEUI, original
event time, counter, FPort and canonical decoded object. Neither boot time nor
counter alone identifies an event. Unknown, ambiguous or unassigned Trackers
produce rejected device events without creating assignments or positions.

Debug uplinks occur approximately every 15 seconds and contain simulated tracker
coordinates. There is no debug flag in this envelope: use an isolated test system
or clearly named test trip. Production alert thresholds remain unchanged.

See [the mockmqtt guide](../mockmqtt/README.md) for Windows/Linux test publishers,
and [ChirpStack MQTT documentation](https://www.chirpstack.io/docs/chirpstack/integrations/mqtt.html)
for server topic configuration. The recorded sample is preserved as an automated
test fixture in `backend/tests/fixtures/chirpstack-uplink.json`.
