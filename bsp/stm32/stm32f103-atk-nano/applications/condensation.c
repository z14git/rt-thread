/**
 * @file condensation.c
 * @author z14git
 * @brief 结露传感器
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

static struct fro_module condensation;
static char              buf[12];

static int condensation_init(void)
{
    return adc_init(ADC_CHANNEL_0);
}

static void condensation_run(struct rt_work *work, void *param)
{
}

static void condensation_deinit(void)
{
    adc_deinit();
}

static int condensation_read(void *cmd, void *data)
{
    uint16_t value;
    value = get_adc(ADC_CHANNEL_0);

    if (value < 4000) {
        rt_snprintf(buf, 12, "%d", value);
    } else {
        value = 68;
        rt_snprintf(buf, 12, "%d", value);
    }

    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops condensation_ops = {
    condensation_init,
    condensation_deinit,
    condensation_run,
    RT_NULL,
    condensation_read,
};

static int condensation_module_init(void)
{
    condensation.ops  = &condensation_ops;
    condensation.type = M_CONDENSATION;
    condensation.name = "结露传感器";

    fro_module_register(&condensation);
    return 0;
}
INIT_COMPONENT_EXPORT(condensation_module_init);
