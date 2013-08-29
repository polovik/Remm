#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "i2c.h"
#include "adxl345.h"

static float axes_scale;

int init_adxl345(adxl345_dataRate_e data_rate, adxl345_range_e range)
{
    int ret;
    uint8_t data[32];
    uint8_t data_len;
    
    /*  Check chip ID   */
    data[0] = ADXL345_REG_DEVID;
    data_len = 1;
    ret = i2c_write(ADXL345_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't select ADXL345 register 0x%02X.\n",
               __FUNCTION__, ADXL345_REG_DEVID);
        return -1;
    }
    data_len = 1;
    ret = i2c_read(ADXL345_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't read ADXL345 chip ID registers.\n", __FUNCTION__);
        return -1;
    }
    if (data[0] != ADXL345_CHIPID) {
        printf("ERROR %s() Incorrect chip ID(0x%02X) of ADXL345 sensor.\n", __FUNCTION__, data[0]);
        return -1;
    }
    
    /*  Enable measurements */
    data[0] = ADXL345_REG_POWER_CTL;
    data[1] = 0x08;
    data_len = 2;
    ret = i2c_write(ADXL345_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't configure ADXL345 register 0x%02X.\n",
               __FUNCTION__, ADXL345_REG_POWER_CTL);
        return -1;
    }
    
    /*  Update data range in format register    */
    uint8_t format;
    ret = i2c_read_byte(ADXL345_I2C_ADDRESS, ADXL345_REG_DATA_FORMAT, &format, "ADXL345");
    if (ret < 0)
        return -1;
    format &= ~0x0F;
    format |= range;
    // Set FULL-RES bit for enable range scaling */
    format |= 0x08;
    ret = i2c_write_byte(ADXL345_I2C_ADDRESS, ADXL345_REG_DATA_FORMAT, format, "ADXL345");
    if (ret < 0)
        return -1;
    axes_scale = 2 << range;
    
    /*  Set data rate and Normal power mode */
    uint8_t bw_rate = data_rate;
    ret = i2c_write_byte(ADXL345_I2C_ADDRESS, ADXL345_REG_BW_RATE, bw_rate, "ADXL345");
    if (ret < 0)
        return -1;
        
    printf("INFO  %s() ADXL345 is successfully initiated.\n", __FUNCTION__);
    
    return 0;
}

void release_adxl345(int signum)
{
    printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

int adxl345_get_axes(adxl345_axes_t *axes)
{
    int ret;
    short axis_reg;
    uint8_t data[32];
    uint8_t data_len;
    float scale = axes_scale * 2 / 1024.; // ADXL345_MG2G_MULTIPLIER
    
    /*  Reading Axes XYZ */
    data[0] = ADXL345_REG_DATAX0;
    data_len = 1;
    ret = i2c_write(ADXL345_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't select ADXL345 register 0x%02X.\n",
               __FUNCTION__, ADXL345_REG_DATAX0);
        return -1;
    }
    
    data_len = 6;
    ret = i2c_read(ADXL345_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't read ADXL345 axes registers.\n", __FUNCTION__);
        return -1;
    }
    printf("INFO  %s() ADXL345 axes: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
           __FUNCTION__, data[0], data[1], data[2], data[3], data[4], data[5]);
           
    axis_reg = (data[1] << 8) | data[0];
    axes->x = axis_reg * scale;
    axis_reg = (data[3] << 8) | data[2];
    axes->z = axis_reg * scale;
    axis_reg = (data[5] << 8) | data[4];
    axes->y = axis_reg * scale;
    
    printf("INFO  %s() ADXL345 scale = %f\n", __FUNCTION__, scale);
    printf("INFO  %s() ADXL345 axis X = %f\n", __FUNCTION__, axes->x);
    printf("INFO  %s() ADXL345 axis Y = %f\n", __FUNCTION__, axes->y);
    printf("INFO  %s() ADXL345 axis Z = %f\n", __FUNCTION__, axes->z);
    
    return 0;
}

