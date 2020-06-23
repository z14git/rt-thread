/**
 * @file zl_servo.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __ZL_SERVO_H
#define __ZL_SERVO_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zl_servo_configure {
        uint32_t range : 16; ///> pos range
        uint32_t channel : 8;
        uint32_t on_off : 1;
        uint32_t reserved : 7;
        uint16_t pos;
};

struct zl_servo_parameter {
        uint8_t type; ///> 0: pwm servo 1: serial servo
        uint8_t channel;
        void *handler;
};

typedef enum {
        ZL_PWM_SERVO,
        ZL_SERIAL_SERVO,
} zl_servo_type;
struct zl_servo_device {
        struct rt_device parent;
        const struct zl_servo_ops *ops;
        const struct zl_servo_parameter *parameter;
        struct zl_servo_configure config;
};

struct zl_servo_ops {
        int (*set_pos)(struct zl_servo_device *servo, uint16_t pos);
        int (*read_pos)(struct zl_servo_device *servo, uint16_t *pos);
        int (*enable)(struct zl_servo_device *servo, uint8_t enable);
};

int zl_servo_register(struct zl_servo_device *device, const char *name,
                      const struct zl_servo_ops *ops, const void *user_data);

#ifdef __cplusplus
}
#endif

#endif // __ZL_SERVO_H
