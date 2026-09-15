#pragma once
#include <stdint.h>

namespace AppConfig {
constexpr uint32_t reportIntervalMs = 15 * 60 * 1000;
constexpr uint32_t lowPowerIntervalMs = 60 * 60 * 1000;
constexpr uint32_t retryInitialMs = 60 * 1000;
constexpr uint32_t eventRetryMs = 15 * 1000;
constexpr uint32_t firstGpsTimeoutMs = 120 * 1000;
constexpr uint32_t gpsTimeoutMs = 30 * 1000;
constexpr uint8_t uplinkPort = 2;
constexpr uint8_t uplinkDataRate = 3; // AS923 SF9/BW125; suitable for a moving tracker.
}
