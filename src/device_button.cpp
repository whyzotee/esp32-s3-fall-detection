#include <Arduino.h>
#include <device_button.h>
#include <board_pins.h>
#include <deep_sleep.h>
#include <driver/rtc_io.h>

namespace {
RTC_DATA_ATTR bool poweredOff = false;
RTC_DATA_ATTR bool pendingSos = false;
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
bool held = false;
bool pendingOff = false;

[[noreturn]] void sleepOff()
{
    poweredOff = true;
    pendingSos = false;
    DeepSleep::powerOff();
}

void monitor(void *)
{
    bool stable = digitalRead(Board::button) == LOW;
    bool previous = stable;
    uint32_t changedAt = millis();
    uint32_t pressedAt = changedAt;
    for (;;) {
        bool raw = digitalRead(Board::button) == LOW;
        uint32_t now = millis();
        if (raw != previous) { previous = raw; changedAt = now; }
        portENTER_CRITICAL(&lock);
        // Keep awake throughout debounce, not only after a confirmed press.
        held = raw || stable;
        if (raw != stable && uint32_t(now - changedAt) >= 40) {
            stable = raw;
            if (stable) pressedAt = changedAt;
            else {
                auto action = DeviceButton::classify(false, uint32_t(changedAt - pressedAt));
                if (action == DeviceButton::Action::PowerOff) pendingOff = true;
                else if (action == DeviceButton::Action::Sos) pendingSos = true;
                held = false;
            }
        }
        portEXIT_CRITICAL(&lock);
        delay(10);
    }
}
}

namespace DeviceButton {
void begin()
{
    rtc_gpio_deinit(Board::button);
    pinMode(Board::button, INPUT_PULLUP);
    if (poweredOff) {
        // Radio and GPS have not been initialized. Measure until debounced release.
        const uint32_t start = millis();
        uint32_t releasedAt = start;
        bool releasing = false;
        while (true) {
            const uint32_t now = millis();
            if (digitalRead(Board::button) == LOW) releasing = false;
            else {
                if (!releasing) { releasing = true; releasedAt = now; }
                if (uint32_t(now - releasedAt) >= 40) break;
            }
            delay(10);
        }
        if (classify(true, uint32_t(releasedAt - start)) != Action::PowerOn) sleepOff();
        poweredOff = false;
        Serial.println("[POWER] On");
    }
    held = digitalRead(Board::button) == LOW;
    if (xTaskCreate(monitor, "button", 2048, nullptr, 2, nullptr) != pdPASS) {
        // Do not deploy without working button handling.
        while (digitalRead(Board::button) == LOW) delay(10);
        sleepOff();
    }
    service();
}
void service()
{
    while (true) {
        portENTER_CRITICAL(&lock);
        const bool pressed = held;
        const bool turnOff = pendingOff;
        portEXIT_CRITICAL(&lock);
        if (turnOff && !pressed) sleepOff();
        if (!pressed && digitalRead(Board::button) == HIGH) return;
        delay(10);
    }
}
bool sosPending()
{
    portENTER_CRITICAL(&lock);
    bool value = pendingSos;
    portEXIT_CRITICAL(&lock);
    return value;
}
void acknowledgeSos()
{
    portENTER_CRITICAL(&lock);
    pendingSos = false;
    portEXIT_CRITICAL(&lock);
}
}
