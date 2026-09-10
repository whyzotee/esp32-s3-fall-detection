# ESP32-S3 LoRaWAN tracker

The radio uses RadioLib 7.6.0 with SX1262, ABP, AS923, Class A,
unconfirmed uplinks on port 2. No Heltec activation license or Serial
provisioning dialogue is used. The OLED uses the ThingPulse SSD1306 driver.

ADXL345 wiring, thresholds and testing: [docs/ADXL345.md](docs/ADXL345.md).
Sensor events override debug's normal status with status 2, while debug
coordinates remain simulated.

Application-side ChirpStack MQTT topics and actual test data:
[docs/RealTest.md](docs/RealTest.md).

## Debug walking track

Set these constants in `src/main.cpp`:

```cpp
constexpr bool LORA_DEBUG = true;
constexpr uint32_t LORA_DEBUG_INTERVAL_MS = 15000;
```

The first uplink is attempted immediately after initialization. Later attempts
start every 15 seconds, subject to radio errors and LoRaWAN duty-cycle limits.
The ESP32 stays awake, including on initialization/storage failures; GPS is
not required. Debug uses DR5 (SF7/BW125), status 0, and synthetic coordinates
starting at 13.756300, 100.501800. Each step moves at 0.9–1.5 m/s with a gradual
random turn of up to 30.5 degrees. At 15 seconds this is about 13.5–22.5 metres.
Rebooting starts the simulated track again. This is a free walking simulation,
not a route constrained to roads. Its timestamp is elapsed boot time, not UTC.
Serial labels it `[SIMULATED WALK]`; the existing payload has no simulation bit,
so gateway/application data must be treated as test data while debug is enabled.

Set `LORA_DEBUG = false` for deployment: acquire GPS, send at DR3 (SF9/BW125),
complete both receive windows, then deep sleep for 15 minutes (or longer if the
MAC duty-cycle timer requires it). Button GPIO0 and fall GPIO6 retain the
original low/high wakeup polarities. Initialization failures retry while awake.

## Gateway and first migration

The initial channel configuration preserves the old channel-0-only mask:
923.2 MHz. Match the network's AS923 plan and ABP settings; debug requires the
gateway to receive SF7/BW125. A normal multi-channel LoRaWAN gateway can receive
this, but a receiver locked to another spreading factor will need adjustment.
Network MAC commands can subsequently update channels.

The existing device address and ABP keys remain in `src/lora_wan.cpp`.
RadioLib cannot import the old Heltec frame counter automatically. Before the
first migration, establish a matching fresh ABP session/counter on the server
according to that server's procedure. Do not permanently disable counter checks.

Session state is stored as a single NVS blob in namespace `radiolib-abp`.
Before each uplink the next frame counter is committed, so a power cut during
transmission skips a counter instead of reusing it. The complete resulting MAC
state is saved after the receive windows. A power cut inside that window may
lose the latest downlink state, but preserves the uplink counter reservation.
If storage fails, transmission is skipped; incompatible/corrupt saved sessions
are not silently reset. Erasing NVS/flash removes this state and requires server
session coordination again. The serialized format is tied to the pinned
RadioLib version; review persistence before upgrading it.

The 17-byte payload remains compatible with `decoder/TTNDecoder.js`. Latitude
and longitude are little-endian float32. Time fields retain their two-byte
slots, with the previously undefined second byte now zero.

## Payload decoder (TTN / The Things Stack)

Copy the JavaScript below into the device's **Payload formatters → Uplink** page.
Select **Custom JavaScript formatter** and save. Use FPort `2` and the 17-byte
decrypted LoRaWAN application payload, not a raw radio packet.
The `decodeUplink(input)` interface follows [The Things Stack documentation](https://www.thethingsindustries.com/docs/integrations/payload-formatters/javascript/uplink/).

| Byte offset (zero-based) | Data |
| --- | --- |
| 0 | Status: 0 = normal, 1 = button pressed, 2 = free-fall detected / suspected fall |
| 1–4 | Latitude: float32 little-endian |
| 5–8 | Longitude: float32 little-endian |
| 9, 11, 13, 15 | Hour, minute, second, centisecond (1/100 second) |
| 10, 12, 14, 16 | Padding: the current firmware sends 0 |

```javascript
function decodeUplink(input) {
    var bytes = input.bytes;
    if (input.fPort !== 2) {
        return { errors: ["Expected FPort 2"] };
    }
    if (!bytes || bytes.length !== 17) {
        return { errors: ["Expected exactly 17 payload bytes"] };
    }

    // IEEE 754 float32, little-endian; compatible with ES5.1 formatters.
    function float32LE(offset) {
        var bits = (bytes[offset] | (bytes[offset + 1] << 8) |
            (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
        var sign = (bits >>> 31) ? -1 : 1;
        var exponent = (bits >>> 23) & 255;
        var fraction = bits & 0x7fffff;
        if (exponent === 255) return NaN;
        if (exponent === 0) return sign * fraction * Math.pow(2, -149);
        return sign * (1 + fraction / 8388608) * Math.pow(2, exponent - 127);
    }

    function pad2(value) {
        return (value < 10 ? "0" : "") + value;
    }

    var latitude = float32LE(1);
    var longitude = float32LE(5);
    if (!isFinite(latitude) || !isFinite(longitude) ||
        Math.abs(latitude) > 90 || Math.abs(longitude) > 180) {
        return { errors: ["Invalid latitude/longitude"] };
    }

    var events = ["Normal Update", "Button Pressed", "Fall Detected"];
    var warnings = [];
    if (bytes[0] > 2) warnings.push("Unknown status code");
    if (latitude === 0 && longitude === 0) {
        warnings.push("Coordinates are 0,0; GPS fix may be unavailable");
    }
    if (bytes[9] > 23 || bytes[11] > 59 || bytes[13] > 59 || bytes[15] > 99) {
        warnings.push("Time fields are outside the expected range");
    }

    return {
        data: {
            status: bytes[0],
            event: events[bytes[0]] || "Unknown",
            latitude: latitude,
            longitude: longitude,
            hour: bytes[9],
            minute: bytes[11],
            second: bytes[13],
            centisecond: bytes[15],
            time_string: pad2(bytes[9]) + ":" + pad2(bytes[11]) + ":" + pad2(bytes[13])
        },
        warnings: warnings,
        errors: []
    };
}
```

Test with FPort `2` and this hexadecimal payload:

```text
02000060410000C9420C00220038004E00
```

Expected `data` output:

```json
{
  "status": 2,
  "event": "Fall Detected",
  "latitude": 14,
  "longitude": 100.5,
  "hour": 12,
  "minute": 34,
  "second": 56,
  "centisecond": 78,
  "time_string": "12:34:56"
}
```

`Fall Detected` retains the original event name for compatibility, but indicates
a suspected fall based on free-fall, not a confirmed human fall. This script omits
the original decoder's `altitude: 10` because the firmware does not transmit altitude.
The existing `decoder/TTNDecoder.js` file has not been changed.
With debug enabled, coordinates are simulated and time is elapsed boot time, not UTC;
the payload has no flag for the decoder to identify debug mode automatically.
With debug disabled, time comes from GNSS. Button/fall events may use previously
cached coordinates and time rather than a fresh fix.

## Verification before handoff

Build using `pio run`. Flash separately, then check gateway/network-server live
data: unconfirmed `sendReceive=0` means no downlink, not proof of gateway receipt.
Verify consecutive simulated positions, approximately 15-second TX intervals,
and increasing frame counters after reset and power interruption. Finally test
with debug disabled, real GPS, timer wakeup, button wakeup and fall wakeup.
