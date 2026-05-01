#include <Arduino.h>
#include <TinyGPS++.h>
#include <driver/rtc_io.h>

#include <gnss.h>
#include <lora_wan.h>

/* --- Setup Key --- */
uint32_t devAddr = (uint32_t)0x00000000;
uint8_t nwkSKey[] = {};
uint8_t appSKey[] = {};
uint8_t devEui[] = {};
uint8_t appEui[] = {};
uint8_t appKey[] = {};

// /* --- Settings --- */
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_AS923;
DeviceClass_t loraWanClass = CLASS_A;
uint16_t userChannelsMask[6] = {0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
uint32_t appTxDutyCycle = 900000;
bool overTheAirActivation = false;
bool loraWanAdr = false;
bool isTxConfirmed = false;
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;

extern TinyGPSPlus GPS;

static void prepareTxFrame(uint8_t port, uint8_t status)
{
    uint8_t hour, second, minute, centisecond;

    get_location();

    Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
    Serial.print(", LAT: ");
    Serial.print(GPS.location.lat());
    Serial.print(", LON: ");
    Serial.print(GPS.location.lng());
    Serial.println();

    hour = GPS.time.hour();
    minute = GPS.time.minute();
    second = GPS.time.second();
    centisecond = GPS.time.centisecond();
    float lat = GPS.location.lat();
    float lon = GPS.location.lng();

    unsigned char *puc;

    appDataSize = 0;
    puc = (unsigned char *)(&lat);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&lon);
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
void setup_lora_wan_app()
{
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    deviceState = DEVICE_STATE_INIT;
}

void enter_lora_wan_app(uint8_t wake_status)
{
    switch (deviceState)
    {
    case DEVICE_STATE_INIT:
    {
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        LoRaWAN.init(loraWanClass, loraWanRegion);
        break;
    }
    case DEVICE_STATE_JOIN:
    {
        LoRaWAN.join();
        break;
    }
    case DEVICE_STATE_SEND:
    {
        prepareTxFrame(appPort, wake_status);
        LoRaWAN.send();

        Serial.println("DEVICE_STATE_SEND");
        deviceState = DEVICE_STATE_CYCLE;
        break;
    }
    case DEVICE_STATE_CYCLE:
    {
        txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
    }
    case DEVICE_STATE_SLEEP:
    {
        if (loraWanClass == CLASS_A)
        {
#ifdef WIRELESS_MINI_SHELL
            esp_deep_sleep_enable_gpio_wakeup(1 << INT_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
#else
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_INT_PIN, 0);
            esp_sleep_enable_ext1_wakeup(1ULL << FALL_INT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);

            rtc_gpio_pullup_en(FALL_INT_PIN);
            rtc_gpio_pulldown_dis(FALL_INT_PIN);
#endif
        }
        LoRaWAN.sleep(loraWanClass);
        break;
    }
    default:
    {
        deviceState = DEVICE_STATE_INIT;
        break;
    }
    }
}