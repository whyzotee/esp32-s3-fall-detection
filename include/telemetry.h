#pragma once
#include <stdint.h>
#include <stddef.h>

struct Telemetry {
    uint8_t status = 0;
    float lat = 0, lon = 0;
    uint8_t hour = 0, minute = 0, second = 0, centisecond = 0;
};
constexpr size_t TELEMETRY_PAYLOAD_SIZE = 17;
Telemetry read_telemetry(uint8_t status);
void encode_telemetry(const Telemetry &sample, uint8_t *payload);

