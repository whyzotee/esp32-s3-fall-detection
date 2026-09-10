#ifndef LORAWAN_H
#define LORAWAN_H

#include <Arduino.h>
#include <fall_detection.h>

#define LoRa_NSS 8
#define LoRa_SCK 9
#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_RST 12
#define LoRa_BUSY 13
#define DIO1 14

#define BTN_INT_PIN 0

bool setup_lora_wan_app(void);
void enter_lora_wan_app(uint8_t wake_status, bool debug, uint32_t debug_interval_ms);

#endif
