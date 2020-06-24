/**
 * @file zl_motor.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-23
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __ZL_MOTOR_H
#define __ZL_MOTOR_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZL_MOTOR_BRAKE 0xffff

struct zl_motor_configure {
        int16_t speed;
};

struct zl_motor_parameter {
        uint8_t type : 7; ///> see zl_motor_type
        uint8_t inverse : 1;
        uint8_t ch1;
        uint8_t ch2;
        void *handler;
};

typedef enum {
        ZL_DRV8872_MOTOR,
        ZL_TB6612_MOTOR,
} zl_motor_type;

struct zl_motor_device {
        struct rt_device parent;
        const struct zl_motor_ops *ops;
        struct zl_motor_configure config;
};

struct zl_motor_ops {
        int (*set_throttle)(struct zl_motor_device *motor, int16_t speed);
        int (*read_throttle)(struct zl_motor_device *motor, int16_t *speed);
        int (*enable)(struct zl_motor_device *servo, uint8_t enable);
};

int zl_motor_register(struct zl_motor_device *device, const char *name,
                      const struct zl_motor_ops *ops, const void *user_data);

#ifdef __cplusplus
}
#endif

#endif // __ZL_MOTOR_H
