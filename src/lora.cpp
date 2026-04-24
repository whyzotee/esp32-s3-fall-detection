#include <lora.h>
#include <Arduino.h>
#include <LoRaWan_APP.h>

/* --- [1] Setup Key ---
uint32_t devAddr = (uint32_t)0x00000000;
uint8_t nwkSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t devEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
*/

/* --- [2] Settings --- */
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_AS923;
DeviceClass_t loraWanClass = CLASS_A;
uint16_t userChannelsMask[6] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
uint32_t appTxDutyCycle = 15000;
bool overTheAirActivation = false;
bool loraWanAdr = false;
bool isTxConfirmed = false;
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;
RTC_DATA_ATTR int bootCount = 0;

uint32_t txCount = 0;

static void prepareTxFrame(uint8_t port)
{
    uint8_t hour, second, minute, centisecond;
    // Modify the settings based on the actual time when the GPS signal was acquired.
    uint32_t timeout = 0;
    if (bootCount == 0)
    {
        timeout = 120 * 1000; // 120s
    }
    else
    {
        timeout = 12 * 1000; // 12s
    }
    bootCount++;
    Serial.println("Waiting for GPS time FIX ...");
    uint32_t start = millis();
    uint32_t start_1 = millis();
    // while (!GPS.location.isValid())
    // {
    //     do
    //     {
    //         if (Serial1.available())
    //         {
    //             GPS.encode(Serial1.read());
    //         }
    //     } while (GPS.charsProcessed() < 500);

    //     if ((millis() - start_1) > 1 * 1000)
    //     {
    //         start_1 = millis();
    //         Serial.println("GPS.location.isValid()");
    //     }
    //     if ((millis() - start) > timeout)
    //     {
    //         Serial.printf("No GPS data received: check wiring%d:%d", millis(), start);
    //         break;
    //     }
    // }
    // Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
    // Serial.print(", LAT: ");
    // Serial.print(GPS.location.lat());
    // Serial.print(", LON: ");
    // Serial.print(GPS.location.lng());
    // Serial.println();

    // hour = GPS.time.hour();
    // minute = GPS.time.minute();
    // second = GPS.time.second();
    // centisecond = GPS.time.centisecond();
    // float lat = GPS.location.lat();
    // float lon = GPS.location.lng();

    unsigned char *puc;

    appDataSize = 0;
    // puc = (unsigned char *)(&lat);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    // puc = (unsigned char *)(&lon);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];

    puc = (unsigned char *)(&hour);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];

    puc = (unsigned char *)(&minute);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];

    puc = (unsigned char *)(&second);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];

    puc = (unsigned char *)(&centisecond);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
}

void setup_lora(void)
{
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(100);

    deviceState = DEVICE_STATE_INIT;
}

void send_pkt()
{
    Serial.println("LoRaWan Init");
    LoRaWAN.init(loraWanClass, loraWanRegion);
    LoRaWAN.setDefaultDR(5);

    Serial.println("LoRaWan Join");
    LoRaWAN.join();

    Serial.println("LoRaWan SendData");
    prepareTxFrame(appPort);
    LoRaWAN.send();
}
