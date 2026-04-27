#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

#include <lora.h>
#include <deep_sleep.h>

// RTC_DATA_ATTR int bootCount = 0;
void print_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }
}

void go_sleep(void)
{
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);
    // esp_sleep_enable_ext1_wakeup()

    rtc_gpio_pulldown_dis(WAKEUP_GPIO);
    rtc_gpio_pullup_en(WAKEUP_GPIO);

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                   " Seconds");

    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

    Serial.println("Going to sleep now");
    Serial.flush();

    delay(1000);

    // esp_sleep_config_gpio_isolate();
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
    Radio.Sleep();
    pinMode(RADIO_DIO_1, ANALOG);
    pinMode(RADIO_NSS, ANALOG);
    pinMode(RADIO_RESET, ANALOG);
    pinMode(RADIO_BUSY, ANALOG);
    pinMode(LORA_CLK, ANALOG);
    pinMode(LORA_MISO, ANALOG);
    pinMode(LORA_MOSI, ANALOG);

    esp_deep_sleep_start();
}