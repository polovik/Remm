#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "i2c.h"
#include "l3g4200d.h"

static float sensitivity;

int init_l3g4200d(l3g4200d_range_e range)
{
    int ret;
    uint8_t data[32];
    uint8_t data_len;
    
    /*  Check chip ID   */
    data[0] = L3G4200D_REGISTER_WHO_AM_I;
    data_len = 1;
    ret = i2c_write(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't select L3G4200D register 0x%02X.\n",
               __FUNCTION__, L3G4200D_REGISTER_WHO_AM_I);
        return -1;
    }
    data_len = 1;
    ret = i2c_read(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't read L3G4200D chip ID registers.\n", __FUNCTION__);
        return -1;
    }
    if (data[0] != L3G4200D_CHIP_ID) {
        printf("ERROR %s() Incorrect chip ID(0x%02X) of L3G4200D sensor.\n", __FUNCTION__, data[0]);
        return -1;
    }
    
    /*  Set CTRL_REG1 (0x20)
    ====================================================================
    BIT  Symbol    Description                                   Default
    ---  ------    --------------------------------------------- -------
    7-6  DR1/0     Output data rate (100,200,400,800Hz)               00
    5-4  BW1/0     Bandwidth selection                                00
      3  PD        0 = Power-down mode, 1 = normal/sleep mode          0
      2  ZEN       Z-axis enable (0 = disabled, 1 = enabled)           1
      1  YEN       Y-axis enable (0 = disabled, 1 = enabled)           1
      0  XEN       X-axis enable (0 = disabled, 1 = enabled)           1 */
    /*  Switch to normal mode and enable all three channels */
    data[0] = L3G4200D_REGISTER_CTRL_REG1;
    data[1] = 0x0F;
    data_len = 2;
    ret = i2c_write(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't configure L3G4200D register 0x%02X.\n",
               __FUNCTION__, L3G4200D_REGISTER_CTRL_REG1);
        return -1;
    }
    
    /*  Set CTRL_REG2 (0x21)
    ====================================================================
    BIT  Symbol    Description                                   Default
    ---  ------    --------------------------------------------- -------
    5-4  HPM1/0    High-pass filter mode selection                    00
    3-0  HPCF3..0  High-pass filter cutoff frequency selection      0000 */
    /*  Nothing to do ... keep default values */
    
    /*  Set CTRL_REG3 (0x22)
    ====================================================================
    BIT  Symbol    Description                                   Default
    ---  ------    --------------------------------------------- -------
     7  I1_Int1   Interrupt enable on INT1 (0=disable,1=enable)       0
     6  I1_Boot   Boot status on INT1 (0=disable,1=enable)            0
     5  H-Lactive Interrupt active config on INT1 (0=high,1=low)      0
     4  PP_OD     Push-Pull/Open-Drain (0=PP, 1=OD)                   0
     3  I2_DRDY   Data ready on DRDY/INT2 (0=disable,1=enable)        0
     2  I2_WTM    FIFO wtrmrk int on DRDY/INT2 (0=dsbl,1=enbl)        0
     1  I2_ORun   FIFO overrun int on DRDY/INT2 (0=dsbl,1=enbl)       0
     0  I2_Empty  FIFI empty int on DRDY/INT2 (0=dsbl,1=enbl)         0 */
    /*  Nothing to do ... keep default values */
    
    /*  Set CTRL_REG4 (0x23)
    ====================================================================
    BIT  Symbol    Description                                   Default
    ---  ------    --------------------------------------------- -------
     7  BDU       Block Data Update (0=continuous, 1=LSB/MSB)         0
     6  BLE       Big/Little-Endian (0=Data LSB, 1=Data MSB)          0
    5-4  FS1/0     Full scale selection                               00
                      00 = 250 dps
                      01 = 500 dps
                      10 = 2000 dps
                      11 = 2000 dps
    2-1  ST1/0     Self Test Enable (0=Disabled)                      00
     0  SIM       SPI Mode (0=4-wire, 1=3-wire)                       0 */
    /* Adjust resolution if requested */
    data[0] = L3G4200D_REGISTER_CTRL_REG4;
    switch (range) {
    case L3G4200D_RANGE_250DPS:
        data[1] = 0x00;
        sensitivity = L3G4200D_SENSITIVITY_250DPS;
        break;
    case L3G4200D_RANGE_500DPS:
        data[1] = 0x10;
        sensitivity = L3G4200D_SENSITIVITY_500DPS;
        break;
    case L3G4200D_RANGE_2000DPS:
        data[1] = 0x20;
        sensitivity = L3G4200D_SENSITIVITY_2000DPS;
        break;
    default:
        printf("ERROR %s() Invalid range L3G4200D 0x%02X.\n", __FUNCTION__, range);
        return -1;
    }
    data_len = 2;
    ret = i2c_write(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't configure L3G4200D register 0x%02X.\n",
               __FUNCTION__, L3G4200D_REGISTER_CTRL_REG4);
        return -1;
    }
    
    /* Set CTRL_REG5 (0x24)
    ====================================================================
    BIT  Symbol    Description                                   Default
    ---  ------    --------------------------------------------- -------
      7  BOOT      Reboot memory content (0=normal, 1=reboot)          0
      6  FIFO_EN   FIFO enable (0=FIFO disable, 1=enable)              0
      4  HPen      High-pass filter enable (0=disable,1=enable)        0
    3-2  INT1_SEL  INT1 Selection config                              00
    1-0  OUT_SEL   Out selection config                               00 */
    /* Nothing to do ... keep default values */
    
    return 0;
}

void release_l3g4200d(int signum)
{
    printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

int l3g4200d_get_data(l3g4200d_angular_rates_s *rates)
{
    int ret;
    short angular_rate;
    uint8_t data;
    
    printf("INFO  %s() L3G4200D sensitivity = %f\n", __FUNCTION__, sensitivity);
    
    /*  Reading Axes XZY */
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_X_L, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = data;
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_X_H, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = (data << 8) | angular_rate;
    rates->x = angular_rate * sensitivity;
    printf("INFO  %s() L3G4200D angular rate X = %f\n", __FUNCTION__, rates->x);
    
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_Y_L, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = data;
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_Y_H, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = (data << 8) | angular_rate;
    rates->y = angular_rate * sensitivity;
    printf("INFO  %s() L3G4200D angular rate Y = %f\n", __FUNCTION__, rates->y);
    
    
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_Z_L, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = data;
    ret = i2c_read_byte(L3G4200D_I2C_ADDRESS, L3G4200D_REGISTER_OUT_Z_H, &data, "L3G4200D");
    if (ret < 0)
        return -1;
    angular_rate = (data << 8) | angular_rate;
    rates->z = angular_rate * sensitivity;
    printf("INFO  %s() L3G4200D angular rate Z = %f\n", __FUNCTION__, rates->z);
    
    return 0;
}

int l3g4200d_get_temperature(int *temperature)
{
    int ret;
    uint8_t data[32];
    uint8_t data_len;
    
    data[0] = L3G4200D_REGISTER_OUT_TEMP;
    data_len = 1;
    ret = i2c_write(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't select L3G4200D register 0x%02X.\n",
               __FUNCTION__, L3G4200D_REGISTER_OUT_TEMP);
        return -1;
    }
    data_len = 1;
    ret = i2c_read(L3G4200D_I2C_ADDRESS, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() Can't read L3G4200D chip ID registers.\n", __FUNCTION__);
        return -1;
    }
    *temperature = data[0];
    printf("INFO  %s() L3G4200D temperature = %d(0x%02X)\n", __FUNCTION__, *temperature, data[0]);
    
    return 0;
}
