#include <Arduino.h>
#include <TinyGPS++.h>
#include <gnss.h>
#include <telemetry.h>
#include <cstring>

extern TinyGPSPlus GPS;

extern int32_t rtc_lat;
extern int32_t rtc_lon;
extern uint8_t rtc_hour;
extern uint8_t rtc_minute;
extern uint8_t rtc_second;
extern uint8_t rtc_centisecond;

Telemetry read_telemetry(uint8_t status)
{
    Telemetry sample{};
    sample.status = status;
    if (status == 1 || status == 2)
    {
        sample.lat = rtc_lat / 1e6f;
        sample.lon = rtc_lon / 1e6f;
        sample.hour = rtc_hour;
        sample.minute = rtc_minute;
        sample.second = rtc_second;
        sample.centisecond = rtc_centisecond;
    }
    else
    {
        get_location();
        sample.lat = GPS.location.lat();
        sample.lon = GPS.location.lng();
        sample.hour = GPS.time.hour();
        sample.minute = GPS.time.minute();
        sample.second = GPS.time.second();
        sample.centisecond = GPS.time.centisecond();
    }

    Serial.printf("[GPS] %02u:%02u:%02u.%02u, LAT: %.6f, LON: %.6f, STATUS: %u\n",
                  sample.hour, sample.minute, sample.second, sample.centisecond,
                  sample.lat, sample.lon, sample.status);
    return sample;
}

void encode_telemetry(const Telemetry &sample, uint8_t *payload)
{
    static_assert(sizeof(float) == 4, "Payload requires float32");
    memset(payload, 0, TELEMETRY_PAYLOAD_SIZE);
    payload[0] = sample.status;
    // ESP32 stores IEEE-754 floats in little-endian byte order.
    memcpy(payload + 1, &sample.lat, 4);
    memcpy(payload + 5, &sample.lon, 4);
    payload[9] = sample.hour;
    payload[11] = sample.minute;
    payload[13] = sample.second;
    payload[15] = sample.centisecond;
}
