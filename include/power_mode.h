#pragma once
#include <stddef.h>
#include <stdint.h>

namespace PowerMode {
bool enabled();
uint32_t reportInterval();
// FPort 10 commands: 01 00 0F enables and 00 00 00 disables Low Power Mode.
bool handleDownlink(uint8_t port, const uint8_t *data, size_t length);
}
