#pragma once

#include <stdint.h>

namespace OtaManager {
// Stops tracker peripherals, hosts the local updater, then restarts or sleeps.
[[noreturn]] void run(uint32_t sleepIntervalMs);
}
