#ifndef SENSORS_H
#define SENSORS_H

typedef enum {
    SENSOR_ALL_DISABLED   = 0x00,
    SENSOR_ADC            = 0x01,
    SENSOR_ALTITUDE       = 0x02,
    SENSOR_MAGNETOMETER   = 0x04,
    SENSOR_ACCELEROMETER  = 0x08,
    SENSOR_GYROSCOPE      = 0x10
} sensor_type_e;

int init_sensors();
void release_sensors(int signum);

float get_battery_charge();
float get_altitude();
int get_heading();  //  direction
//  http://en.wikipedia.org/wiki/Aircraft_principal_axes
int get_pitch();    //  down/up
int get_roll();     //  left/right

#endif // SENSORS_H
