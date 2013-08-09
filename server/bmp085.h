#ifndef BMP085_H_
#define BMP085_H_

#include <stdint.h>

#define BMP085_I2C_ADDRESS                (0x77)
#define BMP085_CHIPID                	   (0x55)

/*	BMP085 REGISTERS	*/
enum {
	BMP085_REGISTER_CAL_AC1            = 0xAA,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC2            = 0xAC,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC3            = 0xAE,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC4            = 0xB0,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC5            = 0xB2,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC6            = 0xB4,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_B1             = 0xB6,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_B2             = 0xB8,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MB             = 0xBA,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MC             = 0xBC,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MD             = 0xBE,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CHIPID             = 0xD0,
	BMP085_REGISTER_VERSION            = 0xD1,
	BMP085_REGISTER_SOFTRESET          = 0xE0,
	BMP085_REGISTER_CONTROL            = 0xF4,
	BMP085_REGISTER_TEMPDATA           = 0xF6,
	BMP085_REGISTER_PRESSUREDATA       = 0xF6,
	BMP085_REGISTER_READTEMPCMD        = 0x2E,
	BMP085_REGISTER_READPRESSURECMD    = 0x34
};

/*	BMP085 mode (oversampling_setting)	*/
typedef enum {
	BMP085_MODE_ULTRALOWPOWER          = 0,	/**<	4.5ms conversation time */
	BMP085_MODE_STANDARD               = 1,	/**<	7.5ms conversation time */
	BMP085_MODE_HIGHRES                = 2,	/**<	13.5ms conversation time */
	BMP085_MODE_ULTRAHIGHRES           = 3	/**<	25.5ms conversation time */
} bmp085_mode_t;

/*	Struct for store BMP085 calibration data	*/
typedef struct {
	int16_t  ac1;
	int16_t  ac2;
	int16_t  ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t  b1;
	int16_t  b2;
	int16_t  mb;
	int16_t  mc;
	int16_t  md;
} calibration_data;

int init_bmp085(bmp085_mode_t resolution_mode);
void release_bmp085(int signum);

float get_temperature(int *raw_temperature_reg);
float get_pressure(int raw_temperature_reg);
float get_altitude(float pressure, float temp);

#endif /* BMP085_H_ */
