#pragma once
#include <telemetry.h>

namespace DebugMode {
void begin(bool enabled, uint32_t intervalMs);
bool enabled();
uint32_t interval();
Telemetry walking_sample(uint8_t status);
void message(const char *message);
void result(int16_t code);
void update(bool fallPending);
}
