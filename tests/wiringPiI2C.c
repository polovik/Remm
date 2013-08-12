#include "wiringPiI2C.h"
#include "../server/bmp085.h"
#include "../server/hmc5883l.h"

#define I2C_FD	10

#define SWAP_2BYTES(x) (((x & 0xFFFF) >> 8) | ((x & 0xFF) << 8))

static module_e testing_module;

void set_testing_module(module_e module)
{
    testing_module = module;
}

int wiringPiI2CReadReg8(int fd, int reg)
{
	int value = -1;

	if (fd != I2C_FD)
		return -1;

    if ((testing_module == MODULE_BMP085_PRESSURE) || (testing_module == MODULE_BMP085_TEMPERATURE)) {
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
    }
    if (testing_module == MODULE_HMC5883L) {
        switch (reg) {
            case HMC5883L_REGISTER_IDENTIFICATION_A:
                value = 0x48;
                break;
            case HMC5883L_REGISTER_IDENTIFICATION_B:
                value = 0x34;
                break;
            case HMC5883L_REGISTER_IDENTIFICATION_C:
                value = 0x33;
                break;
            default:
                value = -1;
                break;
        }
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
            if (testing_module == MODULE_BMP085_TEMPERATURE)
				value = 27898;
            else if (testing_module == MODULE_BMP085_PRESSURE)
				value = (23843 >> 8) << 8;
			else
				value = -1;
			break;
		default:
			value = -1;
			break;
	}

    if (value != -1)
        return SWAP_2BYTES(value);
    else
        return -1;
}

int wiringPiI2CWriteReg8(int fd, int reg, int data)
{
	int ret = -1;

	if (fd != I2C_FD)
		return -1;

    if ((testing_module == MODULE_BMP085_PRESSURE) || (testing_module == MODULE_BMP085_TEMPERATURE)) {
        switch (reg) {
            case BMP085_REGISTER_CONTROL:
                if (testing_module == MODULE_BMP085_TEMPERATURE)
                    ret = (data == BMP085_REGISTER_READTEMPCMD);
                else if (testing_module == MODULE_BMP085_PRESSURE)
                    ret = (data == BMP085_REGISTER_READPRESSURECMD);
                else
                    ret = -1;
                break;
            default:
                ret = -1;
                break;
        }
	}
    if (testing_module == MODULE_HMC5883L) {
        switch (reg) {
            case HMC5883L_REGISTER_CONFIGURATION_A:
                ret = 1;
                break;
            case HMC5883L_REGISTER_CONFIGURATION_B:
                ret = 1;
                break;
            case HMC5883L_REGISTER_STATUS:
                ret = 1;
                break;
            default:
                ret = -1;
                break;
        }
    }
	return ret;
}

int wiringPiI2CSetup(const int devId)
{
    if ((testing_module == MODULE_BMP085_PRESSURE) || (testing_module == MODULE_BMP085_TEMPERATURE)) {
        if (devId == 0x77)
            return I2C_FD;
    }
    if (testing_module == MODULE_HMC5883L) {
        if (devId == 0x3C)
            return I2C_FD;
    }

	return -1;
}
