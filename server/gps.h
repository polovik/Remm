/*
 * gps.h
 */

#ifndef GPS_H_
#define GPS_H_

int init_gps();
void release_gps(int signum);

int get_gps_pos(double *lat, double *lon);

#endif /* GPS_H_ */
