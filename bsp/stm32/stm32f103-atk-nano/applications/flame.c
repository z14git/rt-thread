/**
 * @file flame.c
 * @author z14git
 * @brief 火焰传感器
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

static struct fro_module flame;
static char              buf[12];

static int flame_init(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT_PULLUP);
    return 0;
}

static void flame_run(struct rt_work *work, void *param)
{
}

static void flame_deinit(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT);
}

static int flame_read(void *cmd, void *data)
{
    if (rt_pin_read(PA0) == PIN_HIGH) {
        rt_snprintf(buf, 12, "无火焰");
    } else {
        rt_snprintf(buf, 12, "有火焰");
    }
    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops flame_ops = {
    flame_init,
    flame_deinit,
    flame_run,
    RT_NULL,
    flame_read,
};

static int flame_module_init(void)
{
    flame.ops  = &flame_ops;
    flame.type = M_FLAME;
    flame.name = "火焰传感器";

    fro_module_register(&flame);
    return 0;
}
INIT_COMPONENT_EXPORT(flame_module_init);
