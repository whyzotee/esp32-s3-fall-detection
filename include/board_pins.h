#pragma once

#include <Arduino.h>

// GPIO numbers, NOT U5 pad numbers. Source: docs/schematic/MCU.jpg.
namespace Board {
constexpr int vextControl = 36; // Active-low; supplies L76L VCC.
constexpr int gnssRx = 41;      // GPS_TXD -> ESP32 RX.
constexpr int gnssTx = 42;      // ESP32 TX -> GPS_RXD.
constexpr int accelCs = 33;
constexpr int accelMiso = 34;
constexpr int accelMosi = 35;
constexpr int accelSck = 37;
constexpr gpio_num_t accelInt1 = GPIO_NUM_7;
constexpr int accelInt2 = 6;    // Connected, but not mapped to an interrupt.
constexpr gpio_num_t button = GPIO_NUM_0;
constexpr int buzzer = 4;
constexpr int vibration = 5;
constexpr int statusLed = 40;
constexpr int batteryAdc = 2;  // VBAT divider: 750k (top) / 360k (bottom).
constexpr int radioCs = 8;
constexpr int radioSck = 9;
constexpr int radioMosi = 10;
constexpr int radioMiso = 11;
constexpr int radioReset = 12;
constexpr int radioBusy = 13;
constexpr int radioDio1 = 14;


void begin();
void setVext(bool enabled);
void prepareSleep();
}
