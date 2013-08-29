#include <stdio.h>
#include "i2c.h"
#include "adc.h"

int init_adc(int rate)
{
    int ret;
    uint8_t chip_id;
    
    ret = i2c_read_byte(ADC_I2C_ADDRESS, ADC_REGISTER_CHIPID, &chip_id, "ADC");
    if (ret < 0)
        return -1;
    if (chip_id != ADC_CHIPID) {
        printf("ERROR %s() Incorrect chip ID(0x%02X) of ADC sensor.\n", __FUNCTION__, chip_id);
        return -1;
    }
    
    printf("INFO  %s() ADC is successfully initiated.\n", __FUNCTION__);
    
    return 0;
}

void release_adc(int signum)
{
    printf("INFO  %s() Resources is released. signum=%d\n", __FUNCTION__, signum);
}

int get_voltage(float *voltage)
{
    int ret;
    uint8_t data[32];
    uint8_t data_len;
    
    data[0] = ADC_REGISTER_VOLTAGE;
    data_len = 1;
    ret = i2c_write(ADC_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't select ADC register 0x%02X.\n",
               __FUNCTION__, ADC_I2C_ADDRESS);
        return -1;
    }
    data_len = 2;
    ret = i2c_read(ADC_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't read ADC voltage registers.\n", __FUNCTION__);
        return -1;
    }
    *voltage = ((data[1] << 8) | data[0]) / 1024.;
    printf("INFO  %s() ADC voltage: 0x%02X, 0x%02X -> %f\n",
           __FUNCTION__, data[0], data[1], *voltage);
           
    return 0;
}
