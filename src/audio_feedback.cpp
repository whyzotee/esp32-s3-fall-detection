#include <Arduino.h>
#include <audio_feedback.h>
#include <board_pins.h>

namespace {
struct Note {
    uint16_t hz;
    uint16_t onMs;
    uint16_t offMs;
};

struct Melody {
    const Note *notes;
    uint8_t count;
    const char *name;
};

// Every state has a recognisably different rhythm, modelled after the
// state-feedback melodies used by commercial badge trackers.
constexpr Note powerOn[] = {
    {1568, 70, 45}, {2093, 70, 45}, {2637, 150, 0},
};
constexpr Note powerOff[] = {
    {2637, 80, 45}, {2093, 80, 45}, {1568, 180, 0},
};
constexpr Note sos[] = {
    {2900, 130, 90}, {2900, 130, 90}, {2900, 130, 0},
};
constexpr Note fall[] = {
    {3300, 160, 70}, {3300, 160, 70}, {3300, 160, 70}, {3300, 320, 0},
};
constexpr Note loraConnected[] = {
    {1760, 70, 40}, {2200, 70, 40}, {2640, 150, 0},
};
constexpr Note otaMode[] = {
    {1200, 100, 55}, {1600, 100, 55}, {1200, 100, 55}, {1600, 100, 55},
    {2400, 220, 0},
};

Melody melodyFor(AudioFeedback::Event event)
{
    switch (event) {
    case AudioFeedback::Event::PowerOn:
        return {powerOn, uint8_t(sizeof(powerOn) / sizeof(powerOn[0])), "power on"};
    case AudioFeedback::Event::PowerOff:
        return {powerOff, uint8_t(sizeof(powerOff) / sizeof(powerOff[0])), "power off"};
    case AudioFeedback::Event::Sos:
        return {sos, uint8_t(sizeof(sos) / sizeof(sos[0])), "SOS"};
    case AudioFeedback::Event::Fall:
        return {fall, uint8_t(sizeof(fall) / sizeof(fall[0])), "fall"};
    case AudioFeedback::Event::LoRaConnected:
        return {loraConnected, uint8_t(sizeof(loraConnected) / sizeof(loraConnected[0])), "LoRa connected"};
    case AudioFeedback::Event::OtaMode:
        return {otaMode, uint8_t(sizeof(otaMode) / sizeof(otaMode[0])), "OTA mode"};
    }
    return {nullptr, 0, "unknown"};
}

QueueHandle_t eventQueue = nullptr;
SemaphoreHandle_t buzzerLock = nullptr;

void playMelody(AudioFeedback::Event event)
{
    const Melody melody = melodyFor(event);
    if (!melody.notes || !buzzerLock) return;
    xSemaphoreTake(buzzerLock, portMAX_DELAY);
    for (uint8_t i = 0; i < melody.count; ++i) {
        const Note &note = melody.notes[i];
        tone(Board::buzzer, note.hz, note.onMs);
        delay(note.onMs);
        noTone(Board::buzzer);
        if (note.offMs) delay(note.offMs);
    }
    xSemaphoreGive(buzzerLock);
}

void worker(void *)
{
    AudioFeedback::Event event;
    for (;;) {
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY) == pdTRUE)
            playMelody(event);
    }
}
}

namespace AudioFeedback {
void begin()
{
    if (eventQueue) return;
    buzzerLock = xSemaphoreCreateMutex();
    eventQueue = xQueueCreate(4, sizeof(Event));
    if (!buzzerLock || !eventQueue ||
        xTaskCreate(worker, "buzzer", 2048, nullptr, 1, nullptr) != pdPASS) {
        Serial.println("[AUDIO] Buzzer worker unavailable");
        eventQueue = nullptr;
    }
}

void play(Event event)
{
    const Melody melody = melodyFor(event);
    if (!melody.notes || !eventQueue) return;
    if (xQueueSend(eventQueue, &event, 0) != pdTRUE) {
        Serial.printf("[AUDIO] Dropped %s melody (queue busy)\n", melody.name);
        return;
    }
    Serial.printf("[AUDIO] %s melody queued\n", melody.name);
}

void playAndWait(Event event)
{
    const Melody melody = melodyFor(event);
    if (!melody.notes || !buzzerLock) return;
    playMelody(event);
}
}
