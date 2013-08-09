#include "wiringPiI2C.h"
#include "../server/bmp085.h"

#define I2C_FD	10

static measure_type_e bmp085_measure_type;

void set_bmp085_measure(measure_type_e type)
{
	bmp085_measure_type = type;
}

int wiringPiI2CReadReg8(int fd, int reg)
{
	int value = -1;

	if (fd != I2C_FD)
		return -1;

	switch (reg) {
		case BMP085_REGISTER_CHIPID:
			value = BMP085_CHIPID;
			break;
		case BMP085_REGISTER_PRESSUREDATA + 2:
			value = (23843 & 0xFF) << 8;
			break;
		default:
			value = -1;
			break;
	}

	return value;
}

int wiringPiI2CReadReg16(int fd, int reg)
{
	int value = -1;

	if (fd != I2C_FD)
		return -1;

	switch (reg) {
		case BMP085_REGISTER_CAL_AC1:
			value = 408;
			break;
		case BMP085_REGISTER_CAL_AC2:
			value = -72;
			break;
		case BMP085_REGISTER_CAL_AC3:
			value = -14383;
			break;
		case BMP085_REGISTER_CAL_AC4:
			value = 32741;
			break;
		case BMP085_REGISTER_CAL_AC5:
			value = 32757;
			break;
		case BMP085_REGISTER_CAL_AC6:
			value = 23153;
			break;
		case BMP085_REGISTER_CAL_B1:
			value = 6190;
			break;
		case BMP085_REGISTER_CAL_B2:
			value = 4;
			break;
		case BMP085_REGISTER_CAL_MB:
			value = -32768;
			break;
		case BMP085_REGISTER_CAL_MC:
			value = -8711;
			break;
		case BMP085_REGISTER_CAL_MD:
			value = 2868;
			break;
		case BMP085_REGISTER_TEMPDATA:	//	BMP085_REGISTER_PRESSUREDATA
			if (bmp085_measure_type == MEASURE_TEMPERATURE_MODE)
				value = 27898;
			else if (bmp085_measure_type == MEASURE_PRESSURE)
				value = (23843 >> 8) << 8;
			else
				value = -1;
			break;
		default:
			value = -1;
			break;
	}

	return value;
}

int wiringPiI2CWriteReg8(int fd, int reg, int data)
{
	int ret = -1;

	if (fd != I2C_FD)
		return -1;

	switch (reg) {
		case BMP085_REGISTER_CONTROL:
			if (bmp085_measure_type == MEASURE_TEMPERATURE_MODE)
				ret = (data == BMP085_REGISTER_READTEMPCMD);
			else if (bmp085_measure_type == MEASURE_PRESSURE)
				ret = (data == BMP085_REGISTER_READPRESSURECMD);
			else
				ret = -1;
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

int wiringPiI2CSetup(const int devId)
{
	if (devId == 0x77)
		return I2C_FD;

	return -1;
}
