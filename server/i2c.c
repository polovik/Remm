#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "i2c.h"

static int i2c_fd = -1;

int init_i2c()
{
    const char *device = "/dev/i2c-1";
    
    errno = 0;
    i2c_fd = open(device, O_RDWR);
    if (i2c_fd < 0) {
        printf("ERROR %s() Can't open I2C interface device %s: %s\n",
               __FUNCTION__, device, strerror(errno));
        return -1;
    }
    return 0;
}

int i2c_write(uint8_t dev_addr, uint8_t *data, uint8_t len)
{
    assert(data != NULL);
    if (i2c_fd < 0) {
        printf("ERROR %s() Can't write to I2C interface\n", __FUNCTION__);
        return -1;
    }
    
    errno = 0;
    if (ioctl(i2c_fd, I2C_SLAVE, dev_addr) < 0) {
        printf("ERROR %s() Can't select I2C interface for device 0x%02X: %s\n",
               __FUNCTION__, dev_addr, strerror(errno));
        return -1;
    }
    
    errno = 0;
    if (write(i2c_fd, data, len) != len) {
        printf("ERROR %s() Can't write %d bytes to I2C device 0x%02X: %s\n",
               __FUNCTION__, len, dev_addr, strerror(errno));
        return -1;
    }
    
    return 0;
}

int i2c_read(uint8_t dev_addr, uint8_t *data, uint8_t len)
{
    assert(data != NULL);
    if (i2c_fd < 0) {
        printf("ERROR %s() Can't read from I2C interface\n", __FUNCTION__);
        return -1;
    }
    
    errno = 0;
    if (ioctl(i2c_fd, I2C_SLAVE, dev_addr) < 0) {
        printf("ERROR %s() Can't select I2C interface for device 0x%02X: %s\n",
               __FUNCTION__, dev_addr, strerror(errno));
        return -1;
    }
    
    errno = 0;
    if (read(i2c_fd, data, len) != len) {
        printf("ERROR %s() Can't read %d bytes from I2C device 0x%02X: %s\n",
               __FUNCTION__, len, dev_addr, strerror(errno));
        return -1;
    }
    
    return 0;
}

int i2c_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t byte, const char *device_name)
{
    int ret;
    uint8_t data[2];
    uint8_t data_len;
    
    data[0] = reg;
    data[1] = byte;
    data_len = 2;
    ret = i2c_write(dev_addr, data, data_len);
    if (ret < 0) {
        printf("ERROR %s() %s: Can't write data in register 0x%02X.\n",
               __FUNCTION__, device_name, reg);
        return -1;
    }
    
    return 0;
}

int i2c_read_byte(uint8_t dev_addr, uint8_t reg, uint8_t *byte, const char *device_name)
{
    int ret;
    
    ret = i2c_write(dev_addr, &reg, 1);
    if (ret < 0) {
        printf("ERROR %s() %s: Can't select register 0x%02X.\n",
               __FUNCTION__, device_name, reg);
        return -1;
    }
    ret = i2c_read(dev_addr, byte, 1);
    if (ret < 0) {
        printf("ERROR %s() %s: Can't read register 0x%02X.\n",
               __FUNCTION__, device_name, reg);
        return -1;
    }
    
    return 0;
}
