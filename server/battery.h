#ifndef BATTERY_H
#define BATTERY_H

#define ADC_I2C_ADDRESS                (0x16)
#define ADC_CHIPID                     (0xAD)

/*	ADC REGISTERS	*/
enum {
    ADC_REGISTER_CHIPID             = 0x10, //  r,  1 byte
    ADC_REGISTER_VOLTAGE            = 0xDD  //  r,  2 bytes
};

int init_battery(int rate);
void release_battery(int signum);
int get_voltage(float *voltage);

#endif // BATTERY_H
