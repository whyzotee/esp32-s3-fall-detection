#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <Arduino.h>

constexpr int ADXL345_SDA_PIN = 4;
constexpr int ADXL345_SCL_PIN = 7;
constexpr gpio_num_t FALL_INT_PIN = GPIO_NUM_6;

// Hardware free-fall is a suspected fall, not a validated human-fall classifier.
// THRESH_FF: 62.5 mg/LSB; TIME_FF: 5 ms/LSB.
constexpr uint8_t FALL_THRESHOLD = 6; // 375 mg on EACH axis
constexpr uint8_t FALL_DURATION = 30; // 150 ms

bool setup_fall_detection();
void update_fall_detection(bool debug);
bool fall_detection_pending();
void acknowledge_fall_detection();
// Leaves the sensor measuring; refuses sleep if an event/read failure is pending.
bool prepare_fall_detection_sleep();

#endif
