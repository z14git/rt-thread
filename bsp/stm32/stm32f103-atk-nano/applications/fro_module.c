/**
 * @file fro_module.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-11-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"

#define PC0 GET_PIN(C, 0)
#define PC1 GET_PIN(C, 1)
#define PC2 GET_PIN(C, 2)
#define PC3 GET_PIN(C, 3)
#define PC4 GET_PIN(C, 4)
#define PC5 GET_PIN(C, 5)

static rt_slist_t _module_list;

const char *fro_module_name = RT_NULL;
char *fro_module_info_str = RT_NULL;

static uint8_t get_fro_module_type(void)
{
    uint8_t module_type;
    uint8_t tmp;
    uint8_t retry = 3;

    do
    {
        module_type = 0;
        module_type |= rt_pin_read(PC0) << 0;
        module_type |= rt_pin_read(PC1) << 1;
        module_type |= rt_pin_read(PC2) << 2;
        module_type |= rt_pin_read(PC3) << 3;
        module_type |= rt_pin_read(PC4) << 4;
        module_type |= rt_pin_read(PC5) << 5;

        rt_thread_mdelay(100);

        tmp = 0;
        tmp |= rt_pin_read(PC0) << 0;
        tmp |= rt_pin_read(PC1) << 1;
        tmp |= rt_pin_read(PC2) << 2;
        tmp |= rt_pin_read(PC3) << 3;
        tmp |= rt_pin_read(PC4) << 4;
        tmp |= rt_pin_read(PC5) << 5;

        if (tmp == module_type)
        {
            return module_type;
        }

    } while (retry--);

    return M_NONE;
}

int fro_module_register(fro_module_t m)
{
    RT_ASSERT(m != RT_NULL);
    RT_ASSERT(m->ops != RT_NULL);
    RT_ASSERT(m->type != RT_NULL);

    rt_slist_append(&_module_list, &m->list);

    return 0;
}

fro_module_t get_current_module(void)
{
    rt_slist_t *module_list = RT_NULL;
    fro_module_t current_module = RT_NULL;
    uint8_t type;

    type = get_fro_module_type();

    rt_slist_for_each(module_list, &_module_list)
    {
        current_module = (fro_module_t)(module_list);
        if (current_module->type == type)
            break;
        current_module = RT_NULL;
    }

    return current_module;
}

static int fro_module_list_init(void)
{
    rt_slist_init(&_module_list);

    return RT_EOK;
}
INIT_BOARD_EXPORT(fro_module_list_init);

static int fro_module_type_init(void)
{
    rt_pin_mode(PC0, PIN_MODE_INPUT);
    rt_pin_mode(PC1, PIN_MODE_INPUT);
    rt_pin_mode(PC2, PIN_MODE_INPUT);
    rt_pin_mode(PC3, PIN_MODE_INPUT);
    rt_pin_mode(PC4, PIN_MODE_INPUT);
    rt_pin_mode(PC5, PIN_MODE_INPUT);
    return 0;
}
INIT_ENV_EXPORT(fro_module_type_init);
