/**
 * @file volume.c
 * @author z14git
 * @brief 声音传感器
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

static struct fro_module volume;
static char              buf[12];

static int volume_init(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT_PULLUP);
    return 0;
}

static void volume_run(struct rt_work *work, void *param)
{
    return;
}

static void volume_deinit(void)
{
    rt_pin_mode(PA0, PIN_MODE_INPUT);
}

static int volume_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "io") == 0) {
            *(double *)data = (rt_pin_read(PA0) == PIN_HIGH) ? 0 : 1;
            return 0;
        }
        return -1;
    } else {
        if (rt_pin_read(PA0) == PIN_HIGH) {
            rt_snprintf(buf, 12, "无声音");
        } else {
            rt_snprintf(buf, 12, "有声音");
        }
        *(char **)data = buf;
    }
    return 0;
}

static const struct fro_module_ops volume_ops = {
    volume_init,
    volume_deinit,
    volume_run,
    RT_NULL,
    volume_read,
};

static int volume_module_init(void)
{
    volume.ops  = &volume_ops;
    volume.type = M_VOLUME;
    volume.name = "声音传感器";

    fro_module_register(&volume);
    return 0;
}
INIT_COMPONENT_EXPORT(volume_module_init);
