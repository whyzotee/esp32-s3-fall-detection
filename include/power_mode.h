#pragma once
#include <stddef.h>
#include <stdint.h>

namespace PowerMode {
bool enabled();
uint32_t reportInterval();
// Only FPort 10 with exactly one byte, 0x01, is defined by the application.
bool handleDownlink(uint8_t port, const uint8_t *data, size_t length);
}
