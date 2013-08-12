#ifndef WIRINGPII2C_H_
#define WIRINGPII2C_H_

typedef enum {
    MODULE_BMP085_TEMPERATURE,
    MODULE_BMP085_PRESSURE,
    MODULE_HMC5883L
} module_e;

void set_testing_module(module_e module);

//int wiringPiI2CRead(int fd)
int wiringPiI2CReadReg8(int fd, int reg);
int wiringPiI2CReadReg16(int fd, int reg);

//int wiringPiI2CWrite          (int fd, int data) ;
int wiringPiI2CWriteReg8(int fd, int reg, int data);
//int wiringPiI2CWriteReg16     (int fd, int reg, int data) ;

//int wiringPiI2CSetupInterface (const char *device, int devId) ;
int wiringPiI2CSetup(const int devId);

#endif /* WIRINGPII2C_H_ */
