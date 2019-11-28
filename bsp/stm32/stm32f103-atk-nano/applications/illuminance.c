/**
 * @file illuminance.c
 * @author z14git
 * @brief 光照传感器
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

static struct fro_module illuminance;
static char              buf[8];

static int illuminance_init(void)
{
    return adc_init(ADC_CHANNEL_0);
}

static void illuminance_run(struct rt_work *work, void *param)
{
}

static void illuminance_deinit(void)
{
    adc_deinit();
}

static int illuminance_read(void *cmd, void *data)
{
    uint16_t value;
    value = get_adc(ADC_CHANNEL_0);
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "adc") == 0) {
            *(double *)data = value;
            return 0;
        }
        return -1;
    } else {
        rt_snprintf(buf, 12, "%d", value);

        *(char **)data = buf;
    }
    return 0;
}

static const struct fro_module_ops illuminance_ops = {
    illuminance_init,
    illuminance_deinit,
    illuminance_run,
    RT_NULL,
    illuminance_read,
};

static int illuminance_module_init(void)
{
    illuminance.ops  = &illuminance_ops;
    illuminance.type = M_ILLUMINANCE;
    illuminance.name = "光照传感器";

    fro_module_register(&illuminance);
    return 0;
}
INIT_COMPONENT_EXPORT(illuminance_module_init);
