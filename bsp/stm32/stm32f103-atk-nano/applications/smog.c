/**
 * @file smog.c
 * @author z14git
 * @brief 烟雾传感器
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
#include "adc.h"

#define SMOG_SENSOR_CHANNEL ADC_CHANNEL_4

static struct fro_module smog;
static char              buf[12];

static int smog_init(void)
{
    return adc_init(SMOG_SENSOR_CHANNEL);
}

static void smog_run(struct rt_work *work, void *param)
{
}

static void smog_deinit(void)
{
    adc_deinit();
}

static int smog_read(void *cmd, void *data)
{
    uint16_t value;
    value = get_adc(SMOG_SENSOR_CHANNEL);

    rt_snprintf(buf, 12, "%d", value);

    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops smog_ops = {
    smog_init,
    smog_deinit,
    smog_run,
    RT_NULL,
    smog_read,
};

static int smog_module_init(void)
{
    smog.ops  = &smog_ops;
    smog.type = M_SMOG;
    smog.name = "烟雾传感器";

    fro_module_register(&smog);
    return 0;
}
INIT_COMPONENT_EXPORT(smog_module_init);
