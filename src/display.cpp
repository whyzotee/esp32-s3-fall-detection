#include <display.h>
#include <SSD1306Wire.h>

static SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64, I2C_ONE, 500000);

void setup_display(void)
{
    pinMode(RST_OLED, OUTPUT);
    digitalWrite(RST_OLED, LOW);
    delay(10);
    digitalWrite(RST_OLED, HIGH);
    display.init();
    display.setFont(ArialMT_Plain_10);
}

void clear_display(void)
{
    display.clear();
}

void display_data(void)
{
    static uint32_t lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500)
    {
        lastDisplayUpdate = millis();

        // if (GPS.location.isValid())
        // {
        //     display.drawString(0, 15, "FIXED: " + String(GPS.satellites.value()) + " Sats");
        //     display.drawString(0, 27, "LAT: " + String(GPS.location.lat(), 6));
        //     display.drawString(0, 39, "LON: " + String(GPS.location.lng(), 6));
        // }
        // else
        // {
        //     display.drawString(0, 15, "SEARCHING GPS...");
        //     display.drawString(0, 27, "Chars: " + String(GPS.charsProcessed()));
        // }
        display.display();
    }
}
