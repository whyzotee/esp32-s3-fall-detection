#include <power_mode.h>
#include <app_config.h>
#include <esp_attr.h>

namespace {
RTC_DATA_ATTR uint32_t lowPowerMarker = 0;
constexpr uint32_t enabledMarker = 0x4C505731;
}

namespace PowerMode {
bool enabled() { return lowPowerMarker == enabledMarker; }
uint32_t reportInterval()
{
    return enabled() ? AppConfig::lowPowerIntervalMs : AppConfig::reportIntervalMs;
}
bool handleDownlink(uint8_t port, const uint8_t *data, size_t length)
{
    if (port != 10 || length != 1 || data == nullptr || data[0] != 0x01)
        return false;
    lowPowerMarker = enabledMarker;
    return true;
}
}
