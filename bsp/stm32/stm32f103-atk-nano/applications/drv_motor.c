/**
 * @file drv_motor.c
 * @author z14git
 * @brief !!!使用前请确保pca9685设备已初始化
 * @version 0.1
 * @date 2020-02-17
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "drv_motor.h"
#include <board.h>

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "motor"
    #define LOG_LVL LOG_LVL_ASSERT
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

static void set_throttle(struct tb6612_motor *motor, int16_t speed)
{
    RT_ASSERT(motor);
    RT_ASSERT(motor->pca);
    RT_ASSERT(motor->pin);

    struct pca9685_device *pca_device = motor->pca;
    struct tb6612_pin *    pin        = motor->pin;

    if (speed == 0) {
        // 将其设为自由停止
        pca_device->ops->set_pwm(pca_device, pin->in1, 0, 4096); // always off
        pca_device->ops->set_pwm(pca_device, pin->in2, 0, 4096); // always off
    }
    if (speed > 0) {
        if (speed > 4096)
            speed = 4096;
        if (speed == 4096) {
            pca_device->ops->set_pwm(pca_device,
                                     pin->in2,
                                     4096,
                                     0); // always on
        } else {
            pca_device->ops->set_pwm(pca_device, pin->in2, 0, speed);
        }
        pca_device->ops->set_pwm(pca_device, pin->in1, 0, 4096); // always off
    } else {
        if (speed < -4096)
            speed = -4096;
        if (speed == -4096) {
            pca_device->ops->set_pwm(pca_device,
                                     pin->in1,
                                     4096,
                                     0); // always on
        } else {
            pca_device->ops->set_pwm(pca_device, pin->in1, 0, 0 - speed);
        }
        pca_device->ops->set_pwm(pca_device, pin->in2, 0, 4096); // always off
    }

    motor->speed = speed;
}

static const struct tb6612_motor_ops _ops = {
    .set_throttle = set_throttle,
};

struct tb6612_motor *create_motor_device(struct tb6612_pin *    ppin,
                                         struct pca9685_device *ppca)
{
    RT_ASSERT(ppca);
    RT_ASSERT(ppin);
    struct tb6612_motor *motor = RT_NULL;

    motor = rt_calloc(1, sizeof(struct tb6612_motor));
    if (RT_NULL == motor) {
        LOG_E("no enough memory for creating motor device");
        goto __exit;
    }

    motor->pin = ppin;
    motor->pca = ppca;
    ppca->ops->set_pwm(ppca, ppin->pwm, 4096, 0); //always on
    motor->ops = &_ops;

    return motor;

__exit:
    if (motor != RT_NULL) {
        rt_free(motor);
    }
    return RT_NULL;
}

//TODO: add delete motor device
