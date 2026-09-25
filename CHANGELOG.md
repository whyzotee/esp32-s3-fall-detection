# Changelog

All notable firmware changes are documented here.

## [1.2] - 2026-09-25

### Added

- Distinct GPIO4 buzzer melodies for power on, power off, SOS, suspected fall,
  successful LoRaWAN session setup, and local Wi-Fi OTA/AP mode.
- Battery voltage and estimated percentage in the LoRaWAN uplink payload.

### Changed

- Uplink payload is 15 bytes. Application decoders must use the updated codec.
- Production deep sleep keeps VEXT disabled by default.
