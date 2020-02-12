/**
 * @file drv_pca9685.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-01-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __DRV_PCA9685_H
#define __DRV_PCA9685_H

#include <rtthread.h>
#include <rtdevice.h>

struct pca9685_device
{
    struct rt_i2c_bus_device *bus;
    uint8_t                   i2c_addr;
    const struct pca9685_ops *ops;
};
typedef struct pca9685_device *pca9685_device_t;

struct pca9685_ops
{
    int (*init)(pca9685_device_t dev);
    int (*set_pwm_freq)(pca9685_device_t dev, uint32_t freq_hz);
    int (*set_pwm)(pca9685_device_t dev,
                   uint8_t          channel,
                   uint16_t         on_time,
                   uint16_t         off_time);
    int (*set_all_pwm)(pca9685_device_t dev, uint8_t on_time, uint8_t off_time);
};

struct pca9685_device *pca9685_device_create(const char *bus_name,
                                             uint8_t     i2c_addr);

#endif // __DRV_PCA9685_H
