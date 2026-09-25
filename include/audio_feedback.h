#pragma once

// Audible feedback is intentionally short and is only played while the ESP32
// is awake. Board::prepareSleep() always silences the buzzer before sleep.
namespace AudioFeedback {
enum class Event : unsigned char {
    PowerOn,
    PowerOff,
    Sos,
    Fall,
    LoRaConnected,
    OtaMode,
};

// Start the buzzer worker after Board::begin() has configured GPIO4.
void begin();

// Queue the event's distinct melody without blocking the tracker cycle.
void play(Event event);

// Use for events that immediately enter deep sleep, so the user can hear the
// complete melody before the buzzer rail is shut down.
void playAndWait(Event event);
}
