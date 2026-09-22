#include <gnss.h>
#include <TinyGPS++.h>
#include <device_button.h>
#include <app_config.h>

TinyGPSPlus GPS;

namespace {
bool echoRawNmea = false;
RTC_DATA_ATTR bool hasPreviousFix = false;

void printGnssStatus(uint32_t elapsedMs)
{
    Serial.printf("[GPS] elapsed=%lus chars=%lu checksum_ok=%lu checksum_fail=%lu "
                  "satellites=%lu location=%s\n",
                  static_cast<unsigned long>(elapsedMs / 1000),
                  static_cast<unsigned long>(GPS.charsProcessed()),
                  static_cast<unsigned long>(GPS.passedChecksum()),
                  static_cast<unsigned long>(GPS.failedChecksum()),
                  static_cast<unsigned long>(GPS.satellites.isValid()
                      ? GPS.satellites.value() : 0),
                  GPS.location.isValid() ? "valid" : "invalid");
}

void printGnssFailure()
{
    if (GPS.charsProcessed() == 0) {
        Serial.println("[GPS] No UART data: check VEXT, L76L power, GPIO41 and baud rate");
    } else if (GPS.passedChecksum() == 0) {
        Serial.println("[GPS] UART data received but no valid NMEA: check baud rate and signal integrity");
    } else {
        Serial.println("[GPS] Valid NMEA received but no position fix: check antenna and sky view");
    }
}
}

RTC_DATA_ATTR int32_t rtc_lat = 0;
RTC_DATA_ATTR int32_t rtc_lon = 0;
RTC_DATA_ATTR uint8_t rtc_hour = 0;
RTC_DATA_ATTR uint8_t rtc_minute = 0;
RTC_DATA_ATTR uint8_t rtc_second = 0;
RTC_DATA_ATTR uint8_t rtc_centisecond = 0;

void setup_gnss(bool rawNmeaDebug)
{
    echoRawNmea = rawNmeaDebug;
    Board::setVext(true);
    // RX-only avoids driving the L76-L 2.8 V UART domain from a 3.3 V GPIO.
    Serial1.begin(9600, SERIAL_8N1, Board::gnssRx, -1);
    pinMode(Board::gnssTx, INPUT);
    Serial.printf("[GPS] UART1 RX=%d TX=disabled baud=9600 raw=%s\n",
                  Board::gnssRx, echoRawNmea ? "ON" : "OFF");
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
    const uint32_t timeout = !hasPreviousFix
        ? AppConfig::firstGpsTimeoutMs : AppConfig::gpsTimeoutMs;

    const uint32_t start = millis();
    uint32_t lastStatus = start;
    Serial.printf("[GPS] Acquisition timeout=%lu ms previous_fix=%s\n",
                  static_cast<unsigned long>(timeout), hasPreviousFix ? "yes" : "no");

    while (!GPS.location.isValid())
    {
        while (Serial1.available())
        {
            const char byte = static_cast<char>(Serial1.read());
            GPS.encode(byte);
            if (echoRawNmea) Serial.write(byte);
        }
        delay(1);
        DeviceButton::service();
        // SOS uses cached coordinates if needed; do not wait indefinitely for a new fix.
        if (DeviceButton::sosPending() || DeviceButton::otaPending()) break;

        const uint32_t now = millis();
        if ((now - lastStatus) >= 1000)
        {
            lastStatus = now;
            printGnssStatus(now - start);
        }
        if ((now - start) >= timeout)
        {
            printGnssFailure();
            break;
        }
    }

    if (GPS.location.isValid())
    {
        rtc_lat = GPS.location.lat() * 1e6;
        rtc_lon = GPS.location.lng() * 1e6;
        hasPreviousFix = true;
        Serial.printf("[GPS] Fix acquired: LAT=%.6f LON=%.6f satellites=%lu\n",
                      GPS.location.lat(), GPS.location.lng(),
                      static_cast<unsigned long>(GPS.satellites.isValid()
                          ? GPS.satellites.value() : 0));
    }

    if (GPS.time.isValid())
    {
        rtc_hour = GPS.time.hour();
        rtc_minute = GPS.time.minute();
        rtc_second = GPS.time.second();
        rtc_centisecond = GPS.time.centisecond();
    }
}
