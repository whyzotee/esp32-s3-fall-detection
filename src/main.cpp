#include <Arduino.h>

#include <gps.h>

void setup()
{
    Serial.begin(115200);

    setup_gps();

    Serial.println("Setting AlwaysLocate mode...");
    Serial1.println("$PMTK225,1,3000,12000,18000,72000*16");
}

void loop()
{
    get_location();
}