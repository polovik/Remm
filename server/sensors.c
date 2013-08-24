#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "i2c.h"
#include "adxl345.h"
#include "bmp085.h"
#include "hmc5883l.h"
#include "l3g4200d.h"
#include "adc.h"
#include "sensors.h"

static pthread_t sensors_thread;
static int exit_thread = 0;
//static struct timeval expiry_time;
static float battery_charge = 0;

void *sensors_polling(void *arg)
{
    float voltage;
    float pressure;
    float temperature;
    float altitude;
    int raw_temperature_reg;
    hmc5883l_axes_t magnetic_axes;
    adxl345_axes_t position_axes;
    l3g4200d_angular_rates_s rates;

//    struct timeval started_at, finished_at, elapsed;
//    int charging_duration;

    printf("INFO  %s() Start sensors polling.\n", __FUNCTION__);

    while (exit_thread == 0) {
        usleep(1000000);
//        gettimeofday(&started_at, NULL);
//        gettimeofday(&finished_at, NULL);
//        timersub(&finished_at, &started_at, &elapsed);
//        charging_duration = elapsed.tv_sec * 1000000 + elapsed.tv_usec;

        //  ADC
        get_voltage(&voltage);
        battery_charge = voltage;

        //  Altitude
        temperature = get_temperature(&raw_temperature_reg);
        pressure = get_pressure(raw_temperature_reg);
        altitude = get_altitude(pressure, temperature);
        printf("INFO  %s() Temperature = %f C\n", __FUNCTION__, temperature);
        printf("INFO  %s() Pressure = %f Pa\n", __FUNCTION__, pressure);
        printf("INFO  %s() Altitude = %f m\n", __FUNCTION__, altitude);

        //  Magnetometer
        hmc5883l_get_axes(&magnetic_axes);
        hmc5883l_get_heading(magnetic_axes);

        //  Accelerometer
        adxl345_get_axes(&position_axes);

        //  Gyroscope
        l3g4200d_get_data(&rates);
    }
    printf("INFO  %s() Exit from sensors_polling thread.\n", __FUNCTION__);
    return 0;
}

int init_sensors()
{
    int ret;

    exit_thread = 0;

    if (init_i2c() != 0) {
        printf("ERROR %s() Can't initialize I2C bus\n", __FUNCTION__);
        return -1;
    }

    ret = init_adc(1);
    if (ret != 0) {
        printf("ERROR %s() Can't initialize ADC sensor\n", __FUNCTION__);
        return -1;
    }

    ret = init_bmp085(BMP085_MODE_ULTRAHIGHRES);
    if (ret != 0) {
        printf("ERROR %s() Can't initialize BMP085 sensor (Pressure)\n", __FUNCTION__);
        return -1;
    }

    ret = hmc5883l_self_test();
    if (ret != 0) {
        printf("ERROR %s() Can't test HMC5883L sensor (Magnetometer)\n", __FUNCTION__);
        return -1;
    }
    sleep(1);

    ret = init_hmc5883l(5, 0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT);
    if (ret != 0) {
        printf("ERROR %s() Can't initialize HMC5883L sensor (Magnetometer)\n", __FUNCTION__);
        return -1;
    }

    ret = init_adxl345(ADXL345_DATARATE_100_HZ, ADXL345_RANGE_2_G);
    if (ret != 0) {
        printf("ERROR %s() Can't initialize ADXL345 sensor (Accelerometer)\n", __FUNCTION__);
        return -1;
    }

    ret = init_l3g4200d(L3G4200D_RANGE_2000DPS);
    if (ret != 0) {
        printf("ERROR %s() Can't initialize L3G4200D sensor (Gyroscope)\n", __FUNCTION__);
        return -1;
    }

    ret = pthread_create(&sensors_thread, NULL, sensors_polling, NULL);
    if (ret != 0) {
        perror("Starting sensors thread");
        return -1;
    }

    printf("INFO  %s() Sensors are successfully initiated.\n", __FUNCTION__);

    return 0;
}

void release_sensors(int signum)
{
    printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
    exit_thread = 1;
    if (pthread_join(sensors_thread, NULL) != 0) {
        perror("Joining to sensors_thread");
    }
    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

float get_battery_charge()
{
    return battery_charge;
}
