#ifndef LORA_H
#define LORA_H

#include <RadioLib.h>
// #include <LoRaWan_APP.h>

#define LoRa_NSS 8
#define LoRa_SCK 9
#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_RST 12
#define LoRa_BUSY 13
#define DIO1 14

void setup_lora(void);
uint8_t send_gps_data(int32_t lat, int32_t lng, uint8_t deviceStatus);

#endif