/**
 * @file buzzer.c
 * @author z14git
 * @brief 蜂鸣器模块
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

#define PA0 GET_PIN(A, 0)
static struct fro_module buzzer;

static int buzzer_init(void)
{
    rt_pin_mode(PA0, PIN_MODE_OUTPUT);
    return 0;
}

static void buzzer_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            rt_pin_write(PA0, PIN_HIGH);
            rt_thread_mdelay(25);
            rt_pin_write(PA0, PIN_LOW);
            rt_thread_mdelay(25);
        }
        rt_thread_mdelay(500);
        if (*ptr_work_status == 0)
            break;
    }
}

static void buzzer_deinit(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT);
}

static const struct fro_module_ops buzzer_ops =
    {
        buzzer_init,
        buzzer_deinit,
        buzzer_run,
        RT_NULL,
        RT_NULL,
};

static int buzzer_module_init(void)
{
    buzzer.ops = &buzzer_ops;
    buzzer.type = M_BUZZER;
    buzzer.name = "蜂鸣器模块";

    fro_module_register(&buzzer);
    return 0;
}
INIT_APP_EXPORT(buzzer_module_init);
