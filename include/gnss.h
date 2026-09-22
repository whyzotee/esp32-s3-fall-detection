#ifndef GNSS_H
#define GNSS_H

#include <board_pins.h>

void setup_gnss(bool rawNmeaDebug);
void stop_gnss();
void get_location();

#endif
