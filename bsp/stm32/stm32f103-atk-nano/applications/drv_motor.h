/**
 * @file drv_motor.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-02-17
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __DRV_MOTOR_H
#define __DRV_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtdevice.h>
#include "drv_pca9685.h"

struct tb6612_pin
{
    uint8_t in1;
    uint8_t in2;
    uint8_t pwm;
};

struct tb6612_motor
{
    struct tb6612_pin *            pin;
    struct pca9685_device *        pca;
    int16_t                        speed;
    const struct tb6612_motor_ops *ops;
};

struct tb6612_motor_ops
{
    void (*set_throttle)(struct tb6612_motor *motor, int16_t speed);
};

struct tb6612_motor *create_motor_device(struct tb6612_pin *    ppin,
                                         struct pca9685_device *ppca);

#ifdef __cplusplus
}
#endif

#endif // __DRV_MOTOR_H
