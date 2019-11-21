/**
 * @file ir_reflect.c
 * @author z14git
 * @brief 红外反射传感器
 * @version 0.1
 * @date 2019-11-14
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"

#define SENSOR_PIN GET_PIN(A, 0)

static struct fro_module ir_reflect;
static char              buf[12];

static int ir_reflect_init(void)
{
    rt_pin_mode(SENSOR_PIN, PIN_MODE_INPUT);
    return 0;
}

static void ir_reflect_run(struct rt_work *work, void *param)
{
}

static void ir_reflect_deinit(void)
{
}

static int ir_reflect_read(void *cmd, void *data)
{
    if (rt_pin_read(SENSOR_PIN) == PIN_LOW) {
        rt_snprintf(buf, 12, "有障碍");
    } else {
        rt_snprintf(buf, 12, "无障碍");
    }
    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops ir_reflect_ops = {
    ir_reflect_init,
    ir_reflect_deinit,
    ir_reflect_run,
    RT_NULL,
    ir_reflect_read,
};

static int ir_reflect_module_init(void)
{
    ir_reflect.ops  = &ir_reflect_ops;
    ir_reflect.type = M_IR_REFLECT;
    ir_reflect.name = "红外反射传感器";

    fro_module_register(&ir_reflect);
    return 0;
}
INIT_APP_EXPORT(ir_reflect_module_init);
