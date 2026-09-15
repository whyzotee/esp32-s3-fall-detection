#include <gnss.h>
#include <TinyGPS++.h>
#include <device_button.h>
#include <app_config.h>

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

    Board::setVext(true);
    Serial1.begin(9600, SERIAL_8N1, Board::gnssRx, Board::gnssTx);
}

void stop_gnss()
{
    Serial1.end();
    pinMode(Board::gnssRx, INPUT);
    pinMode(Board::gnssTx, INPUT);
    // Board::prepareSleep turns off the GPS supply afterwards.
}

void get_location()
{
    uint8_t counter = 0;
    const uint32_t timeout = bootCount == 0
        ? AppConfig::firstGpsTimeoutMs : AppConfig::gpsTimeoutMs;

    uint32_t start = millis();
    uint32_t start_1 = millis();

    while (!GPS.location.isValid())
    {
        while (Serial1.available())
        {
            GPS.encode(Serial1.read());
        }
        delay(1);
        DeviceButton::service();
        if (DeviceButton::sosPending()) break;

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
