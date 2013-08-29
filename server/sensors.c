#include <stdio.h>
#include <string.h>
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

typedef struct {
    int worked_sensors;  //  sensor_type_e
    float battery_charge;
    float altitude;
    int heading;
    int pitch;
    int roll;
} indications_t;

static pthread_t sensors_thread;
static int exit_thread = 0;
static indications_t indications;

void *sensors_polling(void *arg)
{
    float voltage;
    float pressure;
    float temperature;
    int raw_temperature_reg;
    hmc5883l_axes_t magnetic_axes;
    adxl345_axes_t position_axes;
    l3g4200d_angular_rates_s rates;
    
    printf("INFO  %s() Start sensors polling.\n", __FUNCTION__);
    
    while (exit_thread == 0) {
        usleep(500000);
        
        //  ADC
        if (indications.worked_sensors & SENSOR_ADC) {
            get_voltage(&voltage);
            indications.battery_charge = voltage;
        }
        
        //  Altitude
        if (indications.worked_sensors & SENSOR_ALTITUDE) {
            temperature = get_temperature(&raw_temperature_reg);
            pressure = get_pressure(raw_temperature_reg);
            indications.altitude = get_altitude(pressure, temperature);
            printf("INFO  %s() Temperature = %f C\n", __FUNCTION__, temperature);
            printf("INFO  %s() Pressure = %f Pa\n", __FUNCTION__, pressure);
            printf("INFO  %s() Altitude = %f m\n", __FUNCTION__, indications.altitude);
        }
        
        //  Magnetometer
        if (indications.worked_sensors & SENSOR_MAGNETOMETER) {
            hmc5883l_get_axes(&magnetic_axes);
            hmc5883l_get_heading(magnetic_axes);
        }
        
        //  Accelerometer
        if (indications.worked_sensors & SENSOR_ACCELEROMETER) {
            adxl345_get_axes(&position_axes);
            indications.pitch = position_axes.y;
            indications.roll = position_axes.x;
        }
        
        //  Gyroscope
        if (indications.worked_sensors & SENSOR_GYROSCOPE) {
            l3g4200d_get_data(&rates);
        }
    }
    printf("INFO  %s() Exit from sensors_polling thread.\n", __FUNCTION__);
    return 0;
}

int init_sensors()
{
    int ret;
    
    exit_thread = 0;
    memset(&indications, 0x00, sizeof(indications_t));
    indications.worked_sensors = SENSOR_ALL_DISABLED;
    
    if (init_i2c() != 0) {
        printf("ERROR %s() Can't initialize I2C bus\n", __FUNCTION__);
        return -1;
    }
    
    ret = init_adc(1);
    if (ret == 0)
        indications.worked_sensors |= SENSOR_ADC;
    else
        printf("ERROR %s() Can't initialize ADC sensor\n", __FUNCTION__);
        
    ret = init_bmp085(BMP085_MODE_ULTRAHIGHRES);
    if (ret == 0)
        indications.worked_sensors |= SENSOR_ALTITUDE;
    else
        printf("ERROR %s() Can't initialize BMP085 sensor (Pressure)\n", __FUNCTION__);
        
    ret = hmc5883l_self_test();
    if (ret == 0) {
        sleep(1);
        ret = init_hmc5883l(5, 0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT);
        if (ret == 0)
            indications.worked_sensors |= SENSOR_MAGNETOMETER;
        else
            printf("ERROR %s() Can't initialize HMC5883L sensor (Magnetometer)\n", __FUNCTION__);
    } else {
        printf("ERROR %s() Can't test HMC5883L sensor (Magnetometer)\n", __FUNCTION__);
    }
    
    ret = init_adxl345(ADXL345_DATARATE_100_HZ, ADXL345_RANGE_2_G);
    if (ret == 0)
        indications.worked_sensors |= SENSOR_ACCELEROMETER;
    else
        printf("ERROR %s() Can't initialize ADXL345 sensor (Accelerometer)\n", __FUNCTION__);
        
    ret = init_l3g4200d(L3G4200D_RANGE_2000DPS);
    if (ret == 0)
        indications.worked_sensors |= SENSOR_GYROSCOPE;
    else
        printf("ERROR %s() Can't initialize L3G4200D sensor (Gyroscope)\n", __FUNCTION__);
        
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
    indications.worked_sensors = SENSOR_ALL_DISABLED;
    exit_thread = 1;
    if (pthread_join(sensors_thread, NULL) != 0) {
        perror("Joining to sensors_thread");
    }
    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

float get_battery_charge()
{
    return indications.battery_charge;
}

float get_altitude()
{
    return indications.altitude;
}

int get_heading()
{
    return indications.heading;
}

int get_pitch()
{
    return indications.pitch;
}

int get_roll()
{
    return indications.roll;
}
