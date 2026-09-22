# Receiving Test Data over MQTT (ChirpStack)

Connect your application's MQTT client to the broker used by the ChirpStack MQTT
integration, using the provided broker address, port, and authentication/TLS settings.
Subscribe to one of the topic filters below. Docker access is not required.

## 1. Subscription Topics

All devices in one application (recommended):

```text
application/APPLICATION_ID/device/+/event/up
```

One device:

```text
application/APPLICATION_ID/device/DEV_EUI/event/up
```

All applications and devices, if permitted by the broker's access rules:

```text
application/+/device/+/event/up
```

Replace `APPLICATION_ID` and `DEV_EUI` with values from ChirpStack or the event's
`deviceInfo.applicationId` and `deviceInfo.devEui` fields. `+` matches one topic level.
These are the default ChirpStack topic patterns; use the configured pattern if customized.
Debug mode attempts an uplink approximately every 15 seconds. A subscription receives
new events; it is not a query for historical data.

Do not use `+/gateway/#` to view decoded sensor values. Those are gateway-level
messages, encoded as binary Protobuf in this setup, so they appear as unreadable
characters. Use `application/+/device/+/event/up` for application events, as described
in the [ChirpStack MQTT documentation](https://www.chirpstack.io/docs/chirpstack/integrations/mqtt.html).

## 2. Read the Received Message

Parse the MQTT message payload as UTF-8 JSON. Read `object` for decoded sensor data,
`deviceInfo.devEui` to identify the device, and `fCnt` to track uplink counters.
The `data` field is Base64-encoded binary application data, not the decoded JSON.

Example `.object` from the test event below:

```json
{
  "status": 0,
  "event": "Normal Update",
  "latitude": 13.758852005004883,
  "longitude": 100.50289154052734,
  "hour": 0,
  "minute": 4,
  "second": 45,
  "centisecond": 17,
  "time_string": "00:04:45"
}
```

## 3. Interpret Results and Troubleshoot

| Field | Meaning in this example |
| --- | --- |
| `fPort: 2` | Port used for this application payload |
| `fCnt: 174` | Uplink frame counter; should continue increasing, with possible skipped values |
| `data` | Base64-encoded application payload after LoRaWAN decryption, not decoded coordinate JSON |
| `object` | Output of the payload decoder configured in the device profile |
| `object.status: 0` | Normal periodic update; 1 = SOS, 2 = free-fall / suspected fall |
| `time` | System event timestamp, distinct from `object.time_string` |
| `object.time_string` | In debug mode, elapsed boot time rather than UTC; coordinates are also simulated |
| `txInfo.frequency: 923200000` | Uplink frequency of 923.2 MHz |
| `spreadingFactor: 7`, `bandwidth: 125000` | SF7 / BW125 kHz, as configured for debug mode |
| `regionConfigId: as923` | Region configuration that processed this event |
| `rxInfo[].rssi`, `snr` | Signal measurements reported by the gateway |
| `rxInfo[].location` | Reported gateway location, not tracker location; tracker coordinates are in `object` |

If no messages arrive, check Device → Events for `up` events and verify the MQTT
integration, broker, topic, and subscription permissions. Gateway Online status
alone does not confirm receipt of an uplink from the board.

If application events are still binary, ask the server administrator to check
`json=true` under `[integration.mqtt]` in `chirpstack.toml`.
Gateway Bridge encoding does not need to change to read application JSON. See the
[ChirpStack configuration documentation](https://www.chirpstack.io/docs/chirpstack/configuration.html).

If `.object` is `null` or missing, inspect the full event and check the payload codec
in the device profile. If status or time values are out of range, check the FPort,
17-byte payload, decoder, and ABP AppSKey against the board configuration. Never
publish keys in logs or documentation. An earlier test had mismatched AppSKey values,
which produced incorrect decoded data even though packets were received.

## Complete Event from the Test

The original JSON below is a recorded example. IDs, timestamps, counters, and
coordinates will differ in subsequent transmissions. For the event structure,
see [ChirpStack event types](https://www.chirpstack.io/docs/chirpstack/integrations/events.html).

```json
{
  "deduplicationId": "c7ae79b7-7806-40d2-a4da-48a4e37446c9",
  "time": "2026-09-10T07:14:22.384670565+00:00",
  "deviceInfo": {
    "tenantId": "7b238e1b-67aa-4ca9-9cd9-8cf8f757c53a",
    "tenantName": "ChirpStack",
    "applicationId": "1f1e46b9-af07-462a-8475-c02cf6225c0b",
    "applicationName": "natural-park-application",
    "deviceProfileId": "0e57b96b-dd9f-4719-8f68-df3397b2ba16",
    "deviceProfileName": "natural-park-profile",
    "deviceName": "devce-001",
    "devEui": "70b3d57ed8005343",
    "deviceClassEnabled": "CLASS_A",
    "tags": {}
  },
  "devAddr": "27fa7da0",
  "adr": false,
  "dr": 5,
  "fCnt": 174,
  "fPort": 2,
  "confirmed": false,
  "data": "AEIkXEF7AclCAAAEAC0AEQA=",
  "object": {
    "second": 45.0,
    "minute": 4.0,
    "longitude": 100.50289154052734,
    "centisecond": 17.0,
    "status": 0.0,
    "event": "Normal Update",
    "time_string": "00:04:45",
    "latitude": 13.758852005004883,
    "hour": 0.0
  },
  "rxInfo": [
    {
      "gatewayId": "b827ebffff7d667b",
      "uplinkId": 57665,
      "nsTime": "2026-09-10T07:14:22.173904487+00:00",
      "rssi": -121,
      "snr": -1.0,
      "location": {
        "latitude": 13.729065895080566,
        "longitude": 100.77578735351562,
        "altitude": 10.0
      },
      "context": "t4/l0A==",
      "crcStatus": "CRC_OK"
    }
  ],
  "txInfo": {
    "frequency": 923200000,
    "modulation": { "lora": { "bandwidth": 125000, "spreadingFactor": 7, "codeRate": "CR_4_5" } }
  },
  "regionConfigId": "as923"
}
```
