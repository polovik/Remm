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

