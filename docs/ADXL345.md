# ADXL345 wiring and suspected-fall detection

Implementation: `include/fall_detection.h` and `src/fall_detection.cpp`.
Uses Arduino Wire directly; no additional sensor library is needed.

## Wiring to Heltec WiFi LoRa 32 V4

| ADXL345 module | Heltec V4 connection |
| --- | --- |
| VCC / supply suitable for 3.3 V | 3V3 (always powered) |
| GND | GND |
| SDA / SDI | GPIO4 |
| SCL / SCLK | GPIO7 |
| INT1 | GPIO6 |
| CS | 3V3, selects I2C |
| SDO / ALT ADDRESS | GND, selects address 0x53 |
| INT2 | Leave disconnected |

These are GPIO labels, not physical header pin numbers. The chosen GPIO4/6/7
are exposed in the [Heltec V4.2 schematic](https://resource.heltec.cn/download/WiFi_LoRa_32_V4/Schematic/WiFi_LoRa_32_V4.2.pdf)
and do not overlap this project's LoRa, OLED or GNSS connections. Check the
printed board revision and header labels when wiring. Avoid GPIO5: newer V4
variants use it for PA control. Other pre-existing board mappings in this
project have not been validated on every V4 revision.

For a bare chip, connect both VS and VDD I/O to 3.3 V. For a breakout, check
its supply/regulator requirements. Pull SDA and SCL up to **3.3 V** (typically
4.7 kohm each if the breakout does not already provide pull-ups). Use short
wires and common ground. Do not use 5 V logic. CS and SDO must not float.
The driver also recognizes 0x1D if SDO is deliberately tied to 3.3 V.
These electrical connections follow the [ADXL345 datasheet, I2C section](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf).

Power the sensor from the always-on **3V3**, not Vext: Vext is switched off
before ESP32 deep sleep. The sensor must remain powered and measuring to wake
the ESP32. Its I2C bus is controller 1 at 100 kHz; the OLED uses controller 0
on GPIO17/18, so initializing the display does not change the sensor bus.

## What is detected

The hardware FREE_FALL interrupt reports a **suspected fall** when every
axis stays below 0.375 g for 150 ms. Constants are in `fall_detection.h`:

```cpp
constexpr uint8_t FALL_THRESHOLD = 6; // 6 * 62.5 mg
constexpr uint8_t FALL_DURATION = 30; // 30 * 5 ms
```

These settings are within the datasheet's suggested starting ranges.
The driver uses 100 Hz measurement, full-resolution +/-16 g, and active-high
INT1. INT1 stays latched until INT_SOURCE is read. On wake the driver reads
the old source before reconfiguring the device. It keeps a software pending
event after clearing the hardware latch, until an uplink completes without
a RadioLib error. Events coalesce while one is pending. This uses free-fall
alone: no impact, posture or inactivity confirmation. Slow collapses may be
missed and dropping the device can cause alerts. It is not a validated
human-fall detector. See Analog Devices' [human-fall detection discussion](https://www.analog.com/en/resources/analog-dialogue/articles/detecting-falls-3-axis-digital-accelerometer.html)
for the additional stages required for classification.

## LoRa and sleep behavior

- A pending sensor event sets payload status to **2**; the existing decoder
  displays `Fall Detected`. That label means suspected free-fall in this firmware.
- With `LORA_DEBUG=true`, simulated walking coordinates continue every 15
  seconds. A real sensor event changes status to 2 on the next eligible uplink;
  its accompanying coordinates are still **simulated**, not the incident location.
- With debug off, INT1 on GPIO6 wakes the ESP32 from deep sleep. The event uses
  the cached GNSS position. It may be stale or zero if no previous fix exists.
- Radio initialization, duty-cycle and send errors keep the event queued.
  Pending events retry at 15-second intervals while awake, respecting MAC limits.
- A hardware event arriving during blocking GPS acquisition or radio TX/RX
  remains latched. The driver checks it before TX and again before sleep.
- Unconfirmed uplink success does not confirm gateway/server receipt. There is
  no application-level acknowledgement or guaranteed alarm delivery.
- No sensor at startup: an error is logged, the tracker continues and ADXL345
  wake is not enabled. Connect it and reboot to initialize. Runtime I2C errors
  are logged; the sleep guard refuses sleep when it cannot clear/check the latch.

## Bench checks

1. Boot with debug on. Confirm `Ready at 0x53`, matching pin assignments and
   one acceleration log per second. At rest, the total acceleration should be
   approximately 1 g, with signs/axes depending on orientation.
2. Test using a secured/padded test fixture for the device, not a person.
   Confirm `[ADXL345] FREE_FALL` and a gateway packet with status 2. Verify a
   later normal packet returns to status 0 after the event has cleared.
3. With debug off, verify timer wake first, then sensor-triggered wake and a
   status-2 uplink. The board should return to sleep without an immediate wake
   loop when INT1 is inactive.
4. Repeat with the sensor absent, failed TX, and an event during an uplink.
   Verify the error logs and pending-event behavior. Finally tune thresholds
   using representative mounting and motion data.

The firmware build can check APIs and linking; electrical operation and
detection performance still require these board-level tests.
