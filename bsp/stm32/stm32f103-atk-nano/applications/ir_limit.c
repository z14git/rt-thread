/**
 * @file ir_limit.c
 * @author z14git
 * @brief 红外对射传感器
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

#define LIMIT_PIN GET_PIN(A, 0)

static struct fro_module ir_limit;
static char              buf[12];

static int ir_limit_init(void)
{
    rt_pin_mode(LIMIT_PIN, PIN_MODE_INPUT_PULLUP);
    return 0;
}

static void ir_limit_run(struct rt_work *work, void *param)
{
}

static void ir_limit_deinit(void)
{
    rt_pin_mode(LIMIT_PIN, PIN_MODE_INPUT);
}

static int ir_limit_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "io") == 0) {
            *(double *)data = (rt_pin_read(LIMIT_PIN) == PIN_HIGH) ? 1 : 0;
            return 0;
        }
        return -1;
    } else {
        if (rt_pin_read(LIMIT_PIN) == PIN_HIGH) {
            rt_snprintf(buf, 12, "有障碍");
        } else {
            rt_snprintf(buf, 12, "无障碍");
        }
        *(char **)data = buf;
    }
    return 0;
}

static const struct fro_module_ops ir_limit_ops = {
    ir_limit_init,
    ir_limit_deinit,
    ir_limit_run,
    RT_NULL,
    ir_limit_read,
};

static int ir_limit_module_init(void)
{
    ir_limit.ops  = &ir_limit_ops;
    ir_limit.type = M_INFRARED_LIMIT;
    ir_limit.name = "红外对射传感器";

    fro_module_register(&ir_limit);
    return 0;
}
INIT_COMPONENT_EXPORT(ir_limit_module_init);
