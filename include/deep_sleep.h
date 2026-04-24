#ifndef DEEP_SLEEP_H
#define DEEP_SLEEP_H

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 30          // Time ESP32 will go to sleep (in seconds)
#define WAKEUP_GPIO GPIO_NUM_0

void print_wakeup_reason(void);
void go_sleep(void);

#endif