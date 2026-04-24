#include <gps.h>
#include <HT_TinyGPS++.h>

TinyGPSPlus GPS;

void setup_gps(void)
{
    pinMode(VGNSS_CTRL, OUTPUT);
    digitalWrite(VGNSS_CTRL, LOW);
    pinMode(GNSS_RTS, OUTPUT);
    digitalWrite(GNSS_RTS, LOW);
    delay(100);
    digitalWrite(GNSS_RTS, HIGH);

    Serial1.begin(9600, SERIAL_8N1, GNSS_TX, GNSS_RX);
    Serial.println("setup GNSS");
}

void get_location()
{
    while (Serial1.available() > 0)
    {
        if (GPS.encode(Serial1.read()))
        {
            if (GPS.location.isValid())
            {
                Serial.print("Location: ");
                // ปริ้นพิกัดละติจูด และ ลองจิจูด
                Serial.print(GPS.location.lat(), 6);
                Serial.print(",");
                Serial.print(GPS.location.lng(), 6);

                Serial.print("  Date/Time: ");
                if (GPS.date.isValid())
                {
                    Serial.print(GPS.date.day());
                    Serial.print("/");
                    Serial.print(GPS.date.month());
                    Serial.print("/");
                    Serial.print(GPS.date.year());
                }

                Serial.println();
            }
        }
    }
}