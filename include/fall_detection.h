#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <Arduino.h>
#include <board_pins.h>

constexpr gpio_num_t FALL_INT_PIN = Board::accelInt1;

// Hardware free-fall is a suspected fall, not a validated human-fall classifier.
// ADXL362: +/-2g (1 mg/LSB), 100 Hz (10 ms/sample).
constexpr uint16_t FALL_THRESHOLD_MG = 375;
constexpr uint16_t FALL_DURATION_SAMPLES = 15; // 150 ms

bool setup_fall_detection(bool debug = false);
void update_fall_detection(bool debug);
bool fall_detection_pending();
void acknowledge_fall_detection();
// Leaves the sensor measuring; refuses sleep if an event/read failure is pending.
bool prepare_fall_detection_sleep();

#endif
