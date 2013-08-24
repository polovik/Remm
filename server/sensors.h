#ifndef SENSORS_H
#define SENSORS_H

int init_sensors();
void release_sensors(int signum);

float get_battery_charge();

#endif // SENSORS_H
