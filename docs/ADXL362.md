# ADXL362 suspected-fall detection

Current implementation: `src/fall_detection.cpp`, `include/fall_detection.h`.
The custom V3 schematic uses **ADXL362 over SPI**, not ADXL345 over I2C.
Connections are listed in [HardwareV3.md](HardwareV3.md): CS33, MISO34,
MOSI35, SCLK37, INT1 GPIO7, INT2 GPIO6, and always-on 3V3 supply.
The dedicated HSPI bus does not reconfigure LoRa's global SPI bus.

The driver configures ±2 g, 100 Hz, normal-power continuous measurement.
Absolute inactivity detection implements free-fall: all three axes must stay
below 375 mg for 15 samples (150 ms). INT1 is active-high and latched until
STATUS is read. This follows the [ADXL362 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adxl362.pdf).

These are starting thresholds, not a validated human-fall classifier.
There is no impact/posture confirmation; drops can trigger it and slow
collapses can be missed. The ±2 g measurement range can saturate impacts.
Do not use this prototype as a sole safety alarm.

On boot, the driver verifies device IDs `AD 1D F2`, reads any latched event
before reconfiguration, and checks configuration writes by reading them back.
A software pending event survives deep sleep and sets payload status 2.
Events coalesce until an unconfirmed uplink finishes without a radio error;
this does not guarantee gateway receipt. Debug continues to use simulated
coordinates even for real sensor events.

Before sleeping, the driver checks the latch again and arms GPIO7 high-level
wakeup. ADXL362 stays powered and measuring during ESP32 sleep. Missing sensor
at boot is logged and disables sensor wake; the tracker continues. Runtime
identity/register errors block sleep. SPI identity checks catch common wiring
faults, but SPI has no acknowledgement or CRC and cannot detect every fault.

Run the hardware checks in [HardwareV3.md](HardwareV3.md), including an event
during blocking GPS/radio work and a failed uplink. Verify status returns to
0 after acknowledgement, and no immediate repeated wake at rest. Host tests
and builds do not validate electrical connections or detection performance.
