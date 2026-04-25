#ifndef GNSS_H
#define GNSS_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define VGNSS_CTRL 34
#define GNSS_TX 39
#define GNSS_RX 38
#define GNSS_Wake 40
#define GNSS_PPS 41
#define GNSS_RTS 42

void setup_gnss(void);
uint8_t get_location(SemaphoreHandle_t serialSem);

#endif