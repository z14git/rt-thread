/**
 * @file fan.c
 * @author z14git
 * @brief 风扇模块
 * @version 0.1
 * @date 2019-11-13
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"

#define FAN_PIN  GET_PIN(A, 0)
#define LED_PIN1 GET_PIN(A, 1)
#define LED_PIN2 GET_PIN(A, 2)
#define LED_PIN3 GET_PIN(A, 3)

static struct fro_module fan;

static inline void fan_off(void)
{
    rt_pin_write(FAN_PIN, PIN_LOW);
}

static inline void fan_on(void)
{
    rt_pin_write(FAN_PIN, PIN_HIGH);
}

static void led_on(void)
{
    rt_pin_write(LED_PIN1, PIN_LOW);
    rt_pin_write(LED_PIN2, PIN_LOW);
    rt_pin_write(LED_PIN3, PIN_LOW);
}

static void led_off(void)
{
    rt_pin_write(LED_PIN1, PIN_HIGH);
    rt_pin_write(LED_PIN2, PIN_HIGH);
    rt_pin_write(LED_PIN3, PIN_HIGH);
}

static int fan_init(void)
{
    rt_pin_mode(FAN_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_PIN1, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_PIN2, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_PIN3, PIN_MODE_OUTPUT);

    fan_off();
    led_off();
    return 0;
}

static void fan_run(struct rt_work *work, void *param)
{
    fan_on();
    led_on();
}

static void fan_deinit(void)
{
    fan_off();
    led_off();
    rt_pin_mode(FAN_PIN, PIN_MODE_INPUT);
    rt_pin_mode(LED_PIN1, PIN_MODE_INPUT);
    rt_pin_mode(LED_PIN2, PIN_MODE_INPUT);
    rt_pin_mode(LED_PIN3, PIN_MODE_INPUT);
}

static const struct fro_module_ops fan_ops = {
    fan_init,
    fan_deinit,
    fan_run,
    RT_NULL,
    RT_NULL,
};

static int fan_module_init(void)
{
    fan.ops  = &fan_ops;
    fan.type = M_FAN;
    fan.name = "风扇模块";

    fro_module_register(&fan);
    return 0;
}
INIT_COMPONENT_EXPORT(fan_module_init);
