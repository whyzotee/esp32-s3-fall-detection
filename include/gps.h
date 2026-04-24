#ifndef GPS_H
#define GPS_H

#define VGNSS_CTRL 34
#define GNSS_TX 39
#define GNSS_RX 38
#define GNSS_Wake 40
#define GNSS_PPS 41
#define GNSS_RTS 42

void setup_gps(void);
void get_location(void);

#endif