#include <gnss.h>
#include <TinyGPS++.h>

TinyGPSPlus GPS;

RTC_DATA_ATTR uint16_t bootCount = 0;
RTC_DATA_ATTR int32_t rtc_lat = 0;
RTC_DATA_ATTR int32_t rtc_lon = 0;
RTC_DATA_ATTR uint8_t rtc_hour = 0;
RTC_DATA_ATTR uint8_t rtc_minute = 0;
RTC_DATA_ATTR uint8_t rtc_second = 0;
RTC_DATA_ATTR uint8_t rtc_centisecond = 0;

void setup_gnss(void)
{

    Serial1.begin(9600, SERIAL_8N1, GNSS_TX, GNSS_RX);

    pinMode(VGNSS_CTRL, OUTPUT);
    digitalWrite(VGNSS_CTRL, LOW);

    // pinMode(GNSS_Wake, OUTPUT);
    // digitalWrite(GNSS_Wake, HIGH);
}

uint8_t print_location(SemaphoreHandle_t serialSem)
{
    while (Serial1.available() > 0)
    {
        GPS.encode(Serial1.read());
    }

    if (GPS.location.isUpdated() && GPS.location.isValid())
    {
        if (xSemaphoreTake(serialSem, portMAX_DELAY))
        {
            Serial.printf("[%02d:%02d:%02d] LAT: %.10f, LON: %.10f\n",
                          GPS.time.hour(), GPS.time.minute(), GPS.time.second(),
                          GPS.location.lat(), GPS.location.lng());
            xSemaphoreGive(serialSem);
        }

        return 1;
    }

    return 0;
}

void get_location()
{
    uint8_t counter = 0;
    uint32_t timeout = 0;
    // uint32_t timeout = 10 * 1000;

    if (bootCount == 0)
    {
        timeout = 120 * 1000; // 120s
    }
    else
    {
        timeout = 30 * 1000; // 30s
    }

    uint32_t start = millis();
    uint32_t start_1 = millis();

    while (!GPS.location.isValid())
    {
        while (Serial1.available())
        {
            GPS.encode(Serial1.read());
        }
        delay(1);

        if ((millis() - start_1) > 1 * 1000)
        {
            counter++;
            start_1 = millis();
            Serial.printf("GPS.location.isValid(%d)", counter);
            Serial.println();
        }
        if ((millis() - start) > timeout)
        {
            Serial.printf("No GPS data received: check wiring%d:%d", millis(), start);
            break;
        }
    }

    if (GPS.location.isValid())
    {
        rtc_lat = GPS.location.lat() * 1e6;
        rtc_lon = GPS.location.lng() * 1e6;
    }

    if (GPS.time.isValid())
    {
        rtc_hour = GPS.time.hour();
        rtc_minute = GPS.time.minute();
        rtc_second = GPS.time.second();
        rtc_centisecond = GPS.time.centisecond();
    }

    bootCount++;
}
