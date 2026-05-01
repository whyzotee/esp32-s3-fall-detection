#include <gnss.h>
#include <TinyGPS++.h>

TinyGPSPlus GPS;

RTC_DATA_ATTR uint16_t bootCount = 0;

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
    uint32_t timeout = 120 * 1000;

    // if (bootCount == 0)
    // {
    //     timeout = 120 * 1000; // 120s
    // }
    // else
    // {
    //     timeout = 30 * 1000; // 45s
    // }

    uint32_t start = millis();
    uint32_t start_1 = millis();

    while (!GPS.location.isValid())
    {
        do
        {
            if (Serial1.available())
            {
                GPS.encode(Serial1.read());
            }
        } while (GPS.charsProcessed() < 500);

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

    bootCount++;
}

// void get_location(SemaphoreHandle_t serialSem)
// {
//     uint32_t timeout = 0;
//     if (bootCount2 == 0)
//     {
//         timeout = 120 * 1000; // 120s
//     }
//     else
//     {
//         timeout = 12 * 1000; // 12s
//     }

//     xSemaphoreTake(serialSem, portMAX_DELAY);
//     Serial.println("Waiting for GPS time FIX ...");
//     xSemaphoreGive(serialSem);

//     uint32_t start = millis();
//     uint32_t start_1 = millis();

//     while (!GPS.location.isValid())
//     {
//         while (Serial1.available() > 0)
//         {
//             GPS.encode(Serial1.read());
//         }

//         vTaskDelay(pdMS_TO_TICKS(10));

//         if ((millis() - start_1) > 1 * 1000)
//         {
//             start_1 = millis();
//             xSemaphoreTake(serialSem, portMAX_DELAY);
//             Serial.println("GPS.location.isValid()");
//             xSemaphoreGive(serialSem);
//         }
//         if ((millis() - start) > timeout)
//         {
//             xSemaphoreTake(serialSem, portMAX_DELAY);
//             Serial.printf("No GPS data received: check wiring%d:%d", millis(), start);
//             xSemaphoreGive(serialSem);
//             break;
//         }
//     }

//     xSemaphoreTake(serialSem, portMAX_DELAY);
//     Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
//     Serial.printf("LAT: %.10f, LON: %.10f\n", GPS.location.lat(), GPS.location.lng());
//     xSemaphoreGive(serialSem);
// }