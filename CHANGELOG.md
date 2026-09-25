# Changelog

All notable firmware changes are documented here.

## [1.2] - 2026-09-25

### Added

- Distinct GPIO4 buzzer melodies for power on, power off, SOS, suspected fall,
  OTAA Join progress, successful LoRaWAN session setup, and local Wi-Fi OTA/AP mode.

### Changed

- Removed the temporary VEXT deep-sleep override; VEXT is always disabled during deep sleep.

## [1.1] - 2026-09-25

### Added

- Battery voltage in millivolts and estimated battery percentage in the
  LoRaWAN uplink payload.

### Changed

- Uplink payload is 15 bytes. Application decoders must use the updated codec.
