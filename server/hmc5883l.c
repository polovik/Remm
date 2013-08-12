#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "hmc5883l.h"

static int i2c_write_fd = -1;
static int i2c_read_fd = -1;
static float scale;

int init_hmc5883l(int field_range, int data_rate, hmc5883l_mode_t mode)
{
    int ret;
    unsigned int chipid = 0;

    i2c_write_fd = wiringPiI2CSetup(HMC5883L_I2C_ADDRESS_W);
    if (i2c_write_fd <= 0) {
        printf("ERROR %s() Can't get access to HMC5883L sensor (write).\n", __FUNCTION__);
        return -1;
    }
    i2c_read_fd = wiringPiI2CSetup(HMC5883L_I2C_ADDRESS_R);
    if (i2c_read_fd <= 0) {
        printf("ERROR %s() Can't get access to HMC5883L sensor (read).\n", __FUNCTION__);
        return -1;
    }

    ret = wiringPiI2CReadReg8(i2c_read_fd, HMC5883L_REGISTER_IDENTIFICATION_A);
    if (ret < 0) {
        printf("ERROR %s() Can't read HMC5883L register 0x%X.\n", __FUNCTION__, HMC5883L_REGISTER_IDENTIFICATION_A);
        return -1;
    }
    chipid = ret << 16;
    ret = wiringPiI2CReadReg8(i2c_read_fd, HMC5883L_REGISTER_IDENTIFICATION_B);
    if (ret < 0) {
        printf("ERROR %s() Can't read HMC5883L register 0x%X.\n", __FUNCTION__, HMC5883L_REGISTER_IDENTIFICATION_B);
        return -1;
    }
    chipid = chipid | (ret << 8);
    ret = wiringPiI2CReadReg8(i2c_read_fd, HMC5883L_REGISTER_IDENTIFICATION_C);
    if (ret < 0) {
        printf("ERROR %s() Can't read HMC5883L register 0x%X.\n", __FUNCTION__, HMC5883L_REGISTER_IDENTIFICATION_C);
        return -1;
    }
    chipid = chipid | ret;
    if (chipid != HMC5883L_CHIPID) {
        printf("ERROR %s() Incorrect chip ID(0x%X) of HMC5883L sensor.\n", __FUNCTION__, chipid);
        return -1;
    }

    /*  Set smoothing period and data rate  */
    if ((data_rate < 0) || (data_rate > 6)) {
        printf("ERROR %s() Incorrect data rate(0x%X) of HMC5883L sensor.\n", __FUNCTION__, data_rate);
        return -1;
    }
    unsigned char conf_reg_a;
    conf_reg_a = (0x3 << 5) | (data_rate << 2); //  8 samples for smoothing
    ret = wiringPiI2CWriteReg8(i2c_write_fd, HMC5883L_REGISTER_CONFIGURATION_A, conf_reg_a);
    if (ret < 0) {
        printf("ERROR %s() Can't write 0x%X to HMC5883L register 0x%X.\n", __FUNCTION__,
                conf_reg_a, HMC5883L_REGISTER_CONFIGURATION_A);
        return -1;
    }

    /*  Set gain factor (magnetic field range)  */
    if ((field_range < 0) || (field_range > 7)) {
        printf("ERROR %s() Incorrect field range(0x%X) of HMC5883L sensor.\n", __FUNCTION__, field_range);
        return -1;
    }
    unsigned char conf_reg_b;
    conf_reg_b = field_range << 5;
    ret = wiringPiI2CWriteReg8(i2c_write_fd, HMC5883L_REGISTER_CONFIGURATION_B, conf_reg_b);
    if (ret < 0) {
        printf("ERROR %s() Can't write 0x%X to HMC5883L register 0x%X.\n", __FUNCTION__,
                conf_reg_a, HMC5883L_REGISTER_CONFIGURATION_B);
        return -1;
    }
    switch (field_range) {
        case 0: scale = 0.73; break;
        case 1: scale = 0.92; break;
        case 2: scale = 1.22; break;
        case 3: scale = 1.52; break;
        case 4: scale = 2.27; break;
        case 5: scale = 2.56; break;
        case 6: scale = 3.03; break;
        case 7: scale = 4.35; break;
        default: return -1;
    }

    /*  Set measurement mode    */
    unsigned char mode_reg;
    if ((mode == HMC5883L_MODE_CONTINUOUS_MEASUREMENT) || (mode == HMC5883L_MODE_SINGLE_MEASUREMENT))
        mode_reg = mode;
    else {
        printf("ERROR %s() Try set incorrect mode(0x%X) HMC5883L.\n", __FUNCTION__, mode);
        return -1;
    }
    ret = wiringPiI2CWriteReg8(i2c_write_fd, HMC5883L_REGISTER_STATUS, mode_reg);
    if (ret < 0) {
        printf("ERROR %s() Can't write 0x%X to HMC5883L register 0x%X.\n", __FUNCTION__,
                mode_reg, HMC5883L_REGISTER_STATUS);
        return -1;
    }

    printf("INFO  %s() HMC5883L is successfully initiated.\n", __FUNCTION__);

    return 0;
}

void release_hmc5883l(int signum)
{
    printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

int get_heading(axes_t axes)
{
//    (scaled_x, scaled_y, scaled_z) = self.getAxes()

//    headingRad = math.atan2(scaled_y, scaled_x)
//    headingRad += self.declination

//    # Correct for reversed heading
//    if(headingRad < 0):
//            headingRad += 2*math.pi

//    # Check for wrap and compensate
//    if(headingRad > 2*math.pi):
//            headingRad -= 2*math.pi

//    # Convert to degrees from radians
//    headingDeg = headingRad * 180/math.pi
//    degrees = math.floor(headingDeg)
//    minutes = round(((headingDeg - degrees) * 60))
//    return (degrees, minutes)
    return -1;
}

int get_axes(axes_t *axes)
{
    int ret;
    int data_ready;
    short axis_reg;

    sleep(1);

    /*  Check status    */
    ret = wiringPiI2CReadReg8(i2c_read_fd, HMC5883L_REGISTER_STATUS);
    if (ret < 0) {
        printf("ERROR %s() Can't read HMC5883L register 0x%X.\n", __FUNCTION__, HMC5883L_REGISTER_STATUS);
        return -1;
    }
    data_ready = ret & 0x01;
    printf("INFO  %s() HMC5883L status: LOCK(%d), READY(%d)", __FUNCTION__, (ret & 0x02) >> 1, data_ready);
    if (data_ready == 0) {
        printf("ERROR %s() HMC5883L data not ready yet.\n", __FUNCTION__);
        return -1;
    }

    /*  Reading Axis X */
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis X MSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = ret << 8;
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis X LSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = axis_reg | ret;
    if (axis_reg == -4096) {
        printf("ERROR %s() HMC5883L Axis X - Out of range. Try choose wider range\n", __FUNCTION__);
        return -1;
    }
    axes->x = axis_reg * scale;

    /*  Reading Axis Z */
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis Z MSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = ret << 8;
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis Z LSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = axis_reg | ret;
    if (axis_reg == -4096) {
        printf("ERROR %s() HMC5883L Axis Z - Out of range. Try choose wider range\n", __FUNCTION__);
        return -1;
    }
    axes->z = axis_reg * scale;

    /*  Reading Axis Y */
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis Y MSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = ret << 8;
    ret = wiringPiI2CRead(i2c_read_fd);
    if (ret < 0) {
        printf("ERROR %s() HMC5883L Can't read axis Y LSB.\n", __FUNCTION__);
        return -1;
    }
    axis_reg = axis_reg | ret;
    if (axis_reg == -4096) {
        printf("ERROR %s() HMC5883L Axis Y - Out of range. Try choose wider range\n", __FUNCTION__);
        return -1;
    }
    axes->y = axis_reg * scale;

    printf("INFO  %s() HMC5883L axis X = %f\n", __FUNCTION__, axes->x);
    printf("INFO  %s() HMC5883L axis Y = %f\n", __FUNCTION__, axes->y);
    printf("INFO  %s() HMC5883L axis Z = %f\n", __FUNCTION__, axes->z);
    return 0;
}
