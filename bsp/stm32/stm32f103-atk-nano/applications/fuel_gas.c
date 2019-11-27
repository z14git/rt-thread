/**
 * @file fuel_gas.c
 * @author z14git
 * @brief 燃气传感器
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

#define FUEL_GAS_SENSOR_CHANNEL ADC_CHANNEL_4

static struct fro_module fuel_gas;
static char              buf[12];

static int fuel_gas_init(void)
{
    return adc_init(FUEL_GAS_SENSOR_CHANNEL);
}

static void fuel_gas_run(struct rt_work *work, void *param)
{
}

static void fuel_gas_deinit(void)
{
    adc_deinit();
}

static int fuel_gas_read(void *cmd, void *data)
{
    uint16_t value;
    value = get_adc(FUEL_GAS_SENSOR_CHANNEL);

    rt_snprintf(buf, 12, "%d", value);

    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops fuel_gas_ops = {
    fuel_gas_init,
    fuel_gas_deinit,
    fuel_gas_run,
    RT_NULL,
    fuel_gas_read,
};

static int fuel_gas_module_init(void)
{
    fuel_gas.ops  = &fuel_gas_ops;
    fuel_gas.type = M_FUEL_GAS;
    fuel_gas.name = "燃气传感器";

    fro_module_register(&fuel_gas);
    return 0;
}
INIT_COMPONENT_EXPORT(fuel_gas_module_init);
