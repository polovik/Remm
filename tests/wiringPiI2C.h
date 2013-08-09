#ifndef WIRINGPII2C_H_
#define WIRINGPII2C_H_

typedef enum {
	MEASURE_TEMPERATURE_MODE,
	MEASURE_PRESSURE
} measure_type_e;

void set_bmp085_measure(measure_type_e type);

//int wiringPiI2CRead(int fd)
int wiringPiI2CReadReg8(int fd, int reg);
int wiringPiI2CReadReg16(int fd, int reg);

//int wiringPiI2CWrite          (int fd, int data) ;
int wiringPiI2CWriteReg8(int fd, int reg, int data);
//int wiringPiI2CWriteReg16     (int fd, int reg, int data) ;

//int wiringPiI2CSetupInterface (const char *device, int devId) ;
int wiringPiI2CSetup(const int devId);

#endif /* WIRINGPII2C_H_ */
