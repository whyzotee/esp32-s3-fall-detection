#include <Arduino.h>
#include <TinyGPS++.h>
#include <driver/rtc_io.h>

#include <gnss.h>
#include <lora_wan.h>

/* --- Setup Key --- */
// uint32_t devAddr = (uint32_t)0x00000000;
// uint8_t nwkSKey[] = {};
// uint8_t appSKey[] = {};
// uint8_t devEui[] = {};
// uint8_t appEui[] = {};
// uint8_t appKey[] = {};

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

extern int32_t rtc_lat;
extern int32_t rtc_lon;
extern uint8_t rtc_hour;
extern uint8_t rtc_minute;
extern uint8_t rtc_second;
extern uint8_t rtc_centisecond;

static void prepareTxFrame(uint8_t port, uint8_t status)
{
    uint8_t hour, second, minute, centisecond;
    float lat, lon;

    if (status == 1 || status == 2)
    {
        // if (rtc_lat == 0 && rtc_lon == 0)
        //     get_location();

        lat = rtc_lat / 1e6f;
        lon = rtc_lon / 1e6f;
        hour = rtc_hour;
        minute = rtc_minute;
        second = rtc_second;
        centisecond = rtc_centisecond;
    }
    else
    {
        get_location();
        lat = GPS.location.lat();
        lon = GPS.location.lng();
        hour = GPS.time.hour();
        minute = GPS.time.minute();
        second = GPS.time.second();
        centisecond = GPS.time.centisecond();
    }

    Serial.printf(" %02d:%02d:%02d.%02d", hour, minute, second, centisecond);
    Serial.print(", LAT: ");
    Serial.print(lat);
    Serial.print(", LON: ");
    Serial.print(lon);
    Serial.printf(", STATUS: %d", status);
    Serial.println();

    unsigned char *puc;

    appDataSize = 0;
    appData[appDataSize++] = status;

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
            pinMode(36, OUTPUT);
            digitalWrite(36, HIGH);

            pinMode(38, ANALOG);
            pinMode(39, ANALOG);
            rtc_gpio_isolate(GPIO_NUM_38);
            rtc_gpio_isolate(GPIO_NUM_39);

            esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_INT_PIN, 0);
            esp_sleep_enable_ext1_wakeup(1ULL << FALL_INT_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);

            rtc_gpio_pulldown_en(FALL_INT_PIN);
            rtc_gpio_pullup_dis(FALL_INT_PIN);
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