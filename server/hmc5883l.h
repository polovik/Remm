#ifndef HMC5883L_H
#define HMC5883L_H

#define HMC5883L_I2C_ADDRESS    (0x1E)
#define HMC5883L_CHIPID         (0x483433) // id registers A,B,C

/*	HMC5883L REGISTERS	*/
enum {
    HMC5883L_REGISTER_CONFIGURATION_A       = 0x00,  // R/W
    HMC5883L_REGISTER_CONFIGURATION_B       = 0x01,  // R/W
    HMC5883L_REGISTER_MODE                  = 0x02,  // R/W
    HMC5883L_REGISTER_DATA_X_MSB            = 0x03,  // R
    HMC5883L_REGISTER_DATA_X_LSB            = 0x04,  // R
    HMC5883L_REGISTER_DATA_Z_MSB            = 0x05,  // R
    HMC5883L_REGISTER_DATA_Z_LSB            = 0x06,  // R
    HMC5883L_REGISTER_DATA_Y_MSB            = 0x07,  // R
    HMC5883L_REGISTER_DATA_Y_LSB            = 0x08,  // R
    HMC5883L_REGISTER_STATUS                = 0x09,  // R
    HMC5883L_REGISTER_IDENTIFICATION_A      = 0x0A,  // R
    HMC5883L_REGISTER_IDENTIFICATION_B      = 0x0B,  // R
    HMC5883L_REGISTER_IDENTIFICATION_C      = 0x0C   // R
};

/*	HMC5883L operation mode	*/
typedef enum {
    HMC5883L_MODE_CONTINUOUS_MEASUREMENT = 0,
    HMC5883L_MODE_SINGLE_MEASUREMENT     = 1,
    HMC5883L_MODE_IDLE_MODE              = 2,
    HMC5883L_MODE_IDLE_MODE2             = 3,
    HMC5883L_SELF_TEST                   = 8
} hmc5883l_mode_t;

/* HMC5883L magnetic field axes values */
typedef struct {
    float x;
    float y;
    float z;
} axes_t;

int init_hmc5883l(int field_range, int data_rate, hmc5883l_mode_t mode);
void release_hmc5883l(int signum);

int hmc5883l_get_status(int *locked, int *ready);
int hmc5883l_get_axes(axes_t *axes);
int hmc5883l_get_heading(axes_t axes);
int hmc5883l_self_test();

#endif // HMC5883L_H
