#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "utils.h"
#include "bmp085.h"

static calibration_data coeffs;
static int i2c_fd = -1;
static bmp085_mode_t mode;

static int readCoefficients()
{
	int ret = 0;
#if 1
	coeffs.ac1 = 408;
	coeffs.ac2 = -72;
	coeffs.ac3 = -14383;
	coeffs.ac4 = 32741;
	coeffs.ac5 = 32757;
	coeffs.ac6 = 23153;
	coeffs.b1  = 6190;
	coeffs.b2  = 4;
	coeffs.mb  = -32768;
	coeffs.mc  = -8711;
	coeffs.md  = 2868;
#else
	coeffs.ac1 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC1);
	coeffs.ac2 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC2);
	coeffs.ac3 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC3);
	coeffs.ac4 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC4);
	coeffs.ac5 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC5);
	coeffs.ac6 = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_AC6);
	coeffs.b1  = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_B1);
	coeffs.b2  = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_B2);
    coeffs.mb  = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_MB);
    coeffs.mc  = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_MC);
    coeffs.md  = wiringPiI2CReadReg16(BMP085_REGISTER_CAL_MD);
#endif

    return ret;
}

static int readRawTemperature(int32_t *raw_temp)
{
	int ret;

#if 1
	*raw_temp = 27898;
#else
	ret = wiringPiI2CWriteReg8(i2c_fd, BMP085_REGISTER_CONTROL, BMP085_REGISTER_READTEMPCMD);
	if(ret < 0) {
		printf("ERROR %s() Can't write 0x%X to BMP085 register 0x%X.\n", __FUNCTION__,
				BMP085_REGISTER_CONTROL, BMP085_REGISTER_READTEMPCMD);
		return -1;
	}
	delayMicroseconds(5);
	ret = wiringPiI2CReadReg16(i2c_fd, BMP085_REGISTER_TEMPDATA);
	if(ret < 0) {
		printf("ERROR %s() Can't read BMP085 register 0x%X.\n", __FUNCTION__, BMP085_REGISTER_TEMPDATA);
		return -1;
	}
	*raw_temp = ret;
#endif

	return 0;
}

static int readRawPressure(int32_t *raw_pressure)
{
	int ret = 0;
#if 1
	*raw_pressure = 23843;
#else
	ret = wiringPiI2CWriteReg8(i2c_fd, BMP085_REGISTER_CONTROL, BMP085_REGISTER_READPRESSURECMD + (mode << 6));
	if(ret < 0) {
		printf("ERROR %s() Can't write 0x%X to BMP085 register 0x%X.\n", __FUNCTION__,
				BMP085_REGISTER_CONTROL, BMP085_REGISTER_READPRESSURECMD);
		return -1;
	}
	switch (mode) 	{
		case BMP085_MODE_ULTRALOWPOWER:
			delayMicroseconds(5);
			break;
		case BMP085_MODE_STANDARD:
			delayMicroseconds(8);
			break;
		case BMP085_MODE_HIGHRES:
			delayMicroseconds(14);
			break;
		case BMP085_MODE_ULTRAHIGHRES:
		default:
			delayMicroseconds(26);
			break;
	}

	ret = wiringPiI2CReadReg16(i2c_fd, BMP085_REGISTER_PRESSUREDATA);
	if(ret < 0) {
		printf("ERROR %s() Can't read BMP085 register 0x%X.\n", __FUNCTION__, BMP085_REGISTER_PRESSUREDATA);
		return -1;
	}
	*raw_pressure = (uint32_t)ret << 8;
	ret = wiringPiI2CReadReg8(i2c_fd, BMP085_REGISTER_PRESSUREDATA + 2);
	if(ret < 0) {
		printf("ERROR %s() Can't read BMP085 register 0x%X.\n", __FUNCTION__, BMP085_REGISTER_PRESSUREDATA + 2);
		return -1;
	}
	*raw_pressure += (uint32_t)ret;
	*raw_pressure >>= (8 - mode);
#endif

	return 0;
}

int init_bmp085()
{
	int ret;

	i2c_fd = wiringPiI2CSetup(BMP085_I2C_ADDRESS);
	if (i2c_fd <= 0) {
		printf("ERROR %s() Can't get access to BMP085 sensor.\n", __FUNCTION__);
	    return -1;
	}

	ret = wiringPiI2CReadReg8(i2c_fd, BMP085_REGISTER_CHIPID);
	if(ret != 0x55) {
		printf("ERROR %s() Incorrect chip ID(0x%X) of BMP085 sensor.\n", __FUNCTION__, ret);
		return -1;
	}

	mode = BMP085_MODE_ULTRALOWPOWER;
	readCoefficients();

	printf("INFO  %s() BMP085 is successfully initiated.\n", __FUNCTION__);

	return 0;
}

void release_bmp085(int signum)
{
	printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
	printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

float get_temperature()
{
	float temperature;
	int32_t raw_temp;	//	UT in datasheet
	int32_t x1, x2, b5;     // following datasheet convention

	if (i2c_fd <= 0)
		return -100.;

	if (readRawTemperature(&raw_temp) < 0)
		return -100.;

	x1 = (raw_temp - (int32_t)coeffs.ac6) * ((int32_t)coeffs.ac5) >> 15;
	x2 = ((int32_t)coeffs.mc << 11) / (x1 + (int32_t)coeffs.md);
	b5 = x1 + x2;
	temperature = (b5 + 8) >> 4;
	temperature /= 10;
	return temperature;
}

float get_pressure()
{
	int32_t  compp = 0;
	int32_t  x1, x2, b5, b6, x3, b3, p;
	uint32_t b4, b7;
	int32_t raw_temp;
	int32_t raw_pressure;

	/* Get the raw pressure and temperature values */
	if (readRawTemperature(&raw_temp) < 0)
		return -1.;
	if (readRawPressure(&raw_pressure) < 0)
		return -1.;

	/* Temperature compensation */
	x1 = (raw_temp - (int32_t)coeffs.ac6) * ((int32_t)coeffs.ac5) >> 15;
	x2 = ((int32_t)coeffs.mc << 11) / (x1 + (int32_t)coeffs.md);
	b5 = x1 + x2;

	/* Pressure compensation */
	b6 = b5 - 4000;
	x1 = (coeffs.b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (coeffs.ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = (((((int32_t)coeffs.ac1) * 4 + x3) << mode) + 2) >> 2;
	x1 = (coeffs.ac3 * b6) >> 13;
	x2 = (coeffs.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (coeffs.ac4 * (uint32_t)(x3 + 32768)) >> 15;
	b7 = ((uint32_t)(raw_pressure - b3) * (50000 >> mode));

	if (b7 < 0x80000000) {
		p = (b7 << 1) / b4;
	} else	{
		p = (b7 / b4) << 1;
	}

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	compp = p + ((x1 + x2 + 3791) >> 4);

	/* Assign compensated pressure value */
	return ((float)compp / 100.);
}

float get_Altitude(float pressure, float temp)
{
	/* Hyposometric formula:                      */
	/*                                            */
	/*     ((P0/P)^(1/5.257) - 1) * (T + 273.15)  */
	/* h = -------------------------------------  */
	/*                   0.0065                   */
	/*                                            */
	/* where: h   = height (in meters)            */
	/*        P0  = sea-level pressure (in hPa)   */
	/*        P   = atmospheric pressure (in hPa) */
	/*        T   = temperature (in °C)           */
	float sea_level = 1013.25;
	float meters = (((float)pow((sea_level/pressure), 0.190223f) - 1.0f)
				    * (temp + 273.15f)) / 0.0065f;

	return meters;
}
