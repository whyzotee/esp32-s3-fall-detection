#include <lora.h>
#include <Arduino.h>

// /* --- [2] Settings --- */
// LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_AS923;
// DeviceClass_t loraWanClass = CLASS_A;
// uint16_t userChannelsMask[6] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
// uint32_t appTxDutyCycle = 15000;
// bool overTheAirActivation = false;a
// bool loraWanAdr = false;
// bool isTxConfirmed = false;
// uint8_t appPort = 2;

// uint8_t confirmedNbTrials = 4;

SX1262 radio = new Module(LoRa_NSS, DIO1, LoRa_RST, LoRa_BUSY);
LoRaWANNode node(&radio, &AS923);

/* --- Setup Key --- */
uint32_t devAddr = (uint32_t)0x00000000;
uint8_t nwkSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t devEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appKey[] = {0x62, 0x3A, 0x9F, 0xD7, 0x68, 0x37, 0x74, 0x64, 0x8B, 0xBF, 0x2C, 0x0E, 0xC4, 0x54, 0xFD, 0x86};

// static void prepareTxFrame(uint8_t port)
// {
//     uint8_t hour, second, minute, centisecond;
//     // Modify the settings based on the actual time when the GPS signal was acquired.
//     uint32_t timeout = 0;
//     if (bootCount == 0)
//     {
//         timeout = 120 * 1000; // 120s
//     }
//     else
//     {
//         timeout = 12 * 1000; // 12s
//     }
//     bootCount++;
//     Serial.println("Waiting for GPS time FIX ...");
//     uint32_t start = millis();
//     uint32_t start_1 = millis();
//     // while (!GPS.location.isValid())
//     // {
//     //     do
//     //     {
//     //         if (Serial1.available())
//     //         {
//     //             GPS.encode(Serial1.read());
//     //         }
//     //     } while (GPS.charsProcessed() < 500);

//     //     if ((millis() - start_1) > 1 * 1000)
//     //     {
//     //         start_1 = millis();
//     //         Serial.println("GPS.location.isValid()");
//     //     }
//     //     if ((millis() - start) > timeout)
//     //     {
//     //         Serial.printf("No GPS data received: check wiring%d:%d", millis(), start);
//     //         break;
//     //     }
//     // }
//     // Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
//     // Serial.print(", LAT: ");
//     // Serial.print(GPS.location.lat());
//     // Serial.print(", LON: ");
//     // Serial.print(GPS.location.lng());
//     // Serial.println();

//     // hour = GPS.time.hour();
//     // minute = GPS.time.minute();
//     // second = GPS.time.second();
//     // centisecond = GPS.time.centisecond();
//     // float lat = GPS.location.lat();
//     // float lon = GPS.location.lng();

//     unsigned char *puc;

//     appDataSize = 0;
//     // puc = (unsigned char *)(&lat);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];
//     appData[appDataSize++] = puc[2];
//     appData[appDataSize++] = puc[3];
//     // puc = (unsigned char *)(&lon);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];
//     appData[appDataSize++] = puc[2];
//     appData[appDataSize++] = puc[3];

//     puc = (unsigned char *)(&hour);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];

//     puc = (unsigned char *)(&minute);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];

//     puc = (unsigned char *)(&second);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];

//     puc = (unsigned char *)(&centisecond);
//     appData[appDataSize++] = puc[0];
//     appData[appDataSize++] = puc[1];
// }

// void lora_init(void)
// {
//     // Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
//     static RadioEvents_t RadioEvents;

//     RadioEvents.TxDone = NULL;
//     RadioEvents.TxTimeout = NULL;
//     RadioEvents.RxDone = NULL;

//     Radio.Init(&RadioEvents);
// }

void setup_lora(void)
{
    int state = radio.begin();

    while (state != RADIOLIB_ERR_NONE)
        Serial.printf("Radio init failed, code %d\n", state);

    Serial.println("SX1262 Ready!");

    node.beginABP(devAddr, NULL, NULL, nwkSKey, appSKey);

    delay(5000);

    int16_t node_status = node.activateABP();

    while (node_status != RADIOLIB_LORAWAN_NEW_SESSION)
    {
        Serial.print("LoRaWan node fail to start");
    }

    radio.sleep();
}

uint8_t send_gps_data(int32_t lat, int32_t lng, uint8_t deviceStatus)
{
    uint8_t payload[9];

    payload[0] = deviceStatus;
    payload[1] = (lat >> 24) & 0xFF;
    payload[2] = (lat >> 16) & 0xFF;
    payload[3] = (lat >> 8) & 0xFF;
    payload[4] = lat & 0xFF;
    payload[5] = (lng >> 24) & 0xFF;
    payload[6] = (lng >> 16) & 0xFF;
    payload[7] = (lng >> 8) & 0xFF;
    payload[8] = lng & 0xFF;

    radio.standby();

    int state = node.sendReceive(payload, sizeof(payload));

    radio.sleep(true);

    if (state == RADIOLIB_LORAWAN_UPLINK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}