#include <avr/interrupt.h>
#include <avr/io.h>
#include "i2c.h"

#define LOCAL_ADDR  (0x16 << 1)
#define ADC_CHIPID  (0xAD)
#define ADC_RATE    (F_CPU / 1024 / 32) // 32 samples per second

typedef enum {
    ADC_REGISTER_CHIPID             = 0x10, //  r,  1 byte
    ADC_REGISTER_VOLTAGE            = 0xDD  //  r,  2 bytes
} register_e;

static register_e requested_register = 0x00;
static u16 voltage = 0;

ISR(TIMER1_OVF_vect)
{
    voltage = (ADCH << 2) | ((ADCL >> 6) & 0x3);
    voltage *= 5;

    TCNT1 = 0x10000 - ADC_RATE;
}

// slave operations
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData) 
{
    if (receiveDataLength >= 1)
        requested_register = receiveData[0];

    PORTD ^= 0x80;
} 

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
    // this function will run when a master somewhere else on the bus
    // addresses us and wishes to read data from us
    u08 transmited_bytes = 0;

    PORTD ^= 0x40;

    switch (requested_register) {
        case ADC_REGISTER_CHIPID:
            transmitData[0] = ADC_CHIPID;
            transmited_bytes = 1;
            break;
        case ADC_REGISTER_VOLTAGE:
            transmitData[0] = voltage & 0x00FF;
            transmitData[1] = (voltage >> 8) & 0x00FF;
            transmited_bytes = 2;
            break;
        default:
            transmited_bytes = 0;
            break;
    }

    return transmited_bytes;
}

int main(void)
{
    DDRD = 0xE0;   //D = 5,6,7 - outputs
    PORTD = 0x00;

    //  Configure timer
    TCCR1A = 0;
    TCCR1B = 5; // Prescaler = 1024
    TCNT1 = 0x10000 - ADC_RATE;
    TIFR = 0;   //  Reset all interrups from T/C1
    TIMSK = 0x04;   // Enable T/C1 overflow interrupt
    GIMSK = 0;  //  Disable external interrupts

    //  Enable ADC
    ADMUX = (0 << REFS0) | (1 << ADLAR) | 0x03; //Make AVcc the reference and select ADC3 channel.
    ADCSRA = (1 << ADEN) | (1 << ADFR) | (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0); //Enable Analog convertor, set prescaler to 16
    ADCSRA |= (1 << ADSC); //start conversion

    // initialize i2c function library
  	i2cInit(); 
  	// set local device address and allow response to general call 
  	i2cSetLocalDeviceAddr(LOCAL_ADDR, TRUE); 
	
    i2cSetSlaveReceiveHandler(i2cSlaveReceiveService);

    // set the Slave Transmit Handler function
    // (this function will run whenever a master somewhere else on the bus
    //  attempts to read data from us as a slave)
    i2cSetSlaveTransmitHandler(i2cSlaveTransmitService);

    while(1) {
        //_delay_ms(100);
	}
}
