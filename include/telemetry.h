#pragma once
#include <stdint.h>
#include <stddef.h>

namespace TelemetryFlags {
constexpr uint8_t gpsFresh = 1 << 0;
constexpr uint8_t lowPowerMode = 1 << 1;
constexpr uint8_t debugSimulation = 1 << 2;
constexpr uint8_t vextHeldOn = 1 << 3;
}

struct Telemetry {
    uint8_t status = 0;
    uint8_t flags = 0;
    float lat = 0, lon = 0;
    uint8_t hour = 0, minute = 0, second = 0, centisecond = 0;
};
constexpr size_t TELEMETRY_PAYLOAD_SIZE = 17;
Telemetry read_telemetry(uint8_t status);
void encode_telemetry(const Telemetry &sample, uint8_t *payload);
