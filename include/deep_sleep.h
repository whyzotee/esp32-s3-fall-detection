#pragma once
#include <stdint.h>

namespace DeepSleep {
void logWakeReason();
// Caller handles button actions before entering either sleep mode.
[[noreturn]] void timed(uint32_t durationMs);
[[noreturn]] void powerOff();
}
