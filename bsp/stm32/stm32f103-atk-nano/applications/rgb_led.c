/**
 * @file rgb_led.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-11-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"

#define B_LED_PIN GET_PIN(A, 2)
#define G_LED_PIN GET_PIN(A, 1)
#define R_LED_PIN GET_PIN(A, 0)

static struct fro_module rgb;

static int rgb_init(void)
{
    rt_pin_mode(B_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(G_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(R_LED_PIN, PIN_MODE_OUTPUT);

    rt_pin_write(B_LED_PIN, PIN_HIGH);
    rt_pin_write(G_LED_PIN, PIN_HIGH);
    rt_pin_write(R_LED_PIN, PIN_HIGH);

    return 0;
}

static void rgb_run(struct rt_work *work, void *param)
{
    for (;;)
    {
        rt_pin_write(B_LED_PIN, PIN_LOW);
        rt_pin_write(G_LED_PIN, PIN_HIGH);
        rt_pin_write(R_LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(B_LED_PIN, PIN_HIGH);
        rt_pin_write(G_LED_PIN, PIN_LOW);
        rt_pin_write(R_LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(B_LED_PIN, PIN_HIGH);
        rt_pin_write(G_LED_PIN, PIN_HIGH);
        rt_pin_write(R_LED_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}

static void rgb_deinit(void)
{
    rt_pin_mode(B_LED_PIN, PIN_MODE_INPUT);
    rt_pin_mode(G_LED_PIN, PIN_MODE_INPUT);
    rt_pin_mode(R_LED_PIN, PIN_MODE_INPUT);
}

static const struct fro_module_ops rgb_ops =
    {
        rgb_init,
        rgb_deinit,
        rgb_run,
        RT_NULL,
        RT_NULL,
};

static int rgb_module_init(void)
{
    rgb.ops = &rgb_ops;
    rgb.type = M_RGB_LIGHT;
    rgb.name = "RGB灯模块";

    fro_module_register(&rgb);
    return 0;
}
INIT_APP_EXPORT(rgb_module_init);
