/**
 * @file human_body.c
 * @author z14git
 * @brief 人体传感器
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

static char              buf[8];
static struct fro_module human_body;

static int human_body_init(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT_PULLUP);
    return 0;
}

static void human_body_run(struct rt_work *work, void *param)
{
}

static void human_body_deinit(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT);
}

static int human_body_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "status") == 0) {
            *(double *)data = (rt_pin_read(PA0) == PIN_HIGH) ? 0 : 1;
            return 0;
        }
        return -1;
    } else {
        if (rt_pin_read(PA0) == PIN_HIGH) {
            rt_snprintf(buf, 12, "无人");
        } else {
            rt_snprintf(buf, 12, "有人");
        }
        *(char **)data = buf;
    }
    return 0;
}

static const struct fro_module_ops human_body_ops = {
    human_body_init,
    human_body_deinit,
    human_body_run,
    RT_NULL,
    human_body_read,
};

static int human_body_module_init(void)
{
    human_body.ops  = &human_body_ops;
    human_body.type = M_BODY;
    human_body.name = "人体传感器";

    fro_module_register(&human_body);
    return 0;
}
INIT_COMPONENT_EXPORT(human_body_module_init);
