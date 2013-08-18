#ifndef L3G4200D_H
#define L3G4200D_H

#define L3G4200D_I2C_ADDRESS         (0x6B)        // 1101001
#define L3G4200D_POLL_TIMEOU         (100)         // Maximum number of read attempts
#define L3G4200D_CHIP_ID             (0b11010011)

#define L3G4200D_SENSITIVITY_250DPS  (0.00875F)      // Roughly 22/256 for fixed point match
#define L3G4200D_SENSITIVITY_500DPS  (0.0175F)       // Roughly 45/256
#define L3G4200D_SENSITIVITY_2000DPS (0.070F)        // Roughly 18/256
#define L3G4200D_DPS_TO_RADS         (0.017453293F)  // degress/s to rad/s multiplier

typedef enum {                                    // DEFAULT    TYPE
    L3G4200D_REGISTER_WHO_AM_I            = 0x0F,   // 11010011   r
    L3G4200D_REGISTER_CTRL_REG1           = 0x20,   // 00000111   rw
    L3G4200D_REGISTER_CTRL_REG2           = 0x21,   // 00000000   rw
    L3G4200D_REGISTER_CTRL_REG3           = 0x22,   // 00000000   rw
    L3G4200D_REGISTER_CTRL_REG4           = 0x23,   // 00000000   rw
    L3G4200D_REGISTER_CTRL_REG5           = 0x24,   // 00000000   rw
    L3G4200D_REGISTER_REFERENCE           = 0x25,   // 00000000   rw
    L3G4200D_REGISTER_OUT_TEMP            = 0x26,   //            r
    L3G4200D_REGISTER_STATUS_REG          = 0x27,   //            r
    L3G4200D_REGISTER_OUT_X_L             = 0x28,   //            r
    L3G4200D_REGISTER_OUT_X_H             = 0x29,   //            r
    L3G4200D_REGISTER_OUT_Y_L             = 0x2A,   //            r
    L3G4200D_REGISTER_OUT_Y_H             = 0x2B,   //            r
    L3G4200D_REGISTER_OUT_Z_L             = 0x2C,   //            r
    L3G4200D_REGISTER_OUT_Z_H             = 0x2D,   //            r
    L3G4200D_REGISTER_FIFO_CTRL_REG       = 0x2E,   // 00000000   rw
    L3G4200D_REGISTER_FIFO_SRC_REG        = 0x2F,   //            r
    L3G4200D_REGISTER_INT1_CFG            = 0x30,   // 00000000   rw
    L3G4200D_REGISTER_INT1_SRC            = 0x31,   //            r
    L3G4200D_REGISTER_TSH_XH              = 0x32,   // 00000000   rw
    L3G4200D_REGISTER_TSH_XL              = 0x33,   // 00000000   rw
    L3G4200D_REGISTER_TSH_YH              = 0x34,   // 00000000   rw
    L3G4200D_REGISTER_TSH_YL              = 0x35,   // 00000000   rw
    L3G4200D_REGISTER_TSH_ZH              = 0x36,   // 00000000   rw
    L3G4200D_REGISTER_TSH_ZL              = 0x37,   // 00000000   rw
    L3G4200D_REGISTER_INT1_DURATION       = 0x38    // 00000000   rw
} l3g4200d_registers_e;

typedef enum {
    L3G4200D_RANGE_250DPS,
    L3G4200D_RANGE_500DPS,
    L3G4200D_RANGE_2000DPS
} l3g4200d_range_e;

typedef struct {
    float x;
    float y;
    float z;
} l3g4200d_angular_rates_s;

int init_l3g4200d(l3g4200d_range_e range);
void release_l3g4200d(int signum);
int l3g4200d_get_data(l3g4200d_angular_rates_s *rates);
int l3g4200d_get_temperature(int *temperature);

#endif // L3G4200D_H
