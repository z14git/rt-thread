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
#include "frotech_protocol.h"

#define PC0 GET_PIN(C, 0)
#define PC1 GET_PIN(C, 1)
#define PC2 GET_PIN(C, 2)
#define PC3 GET_PIN(C, 3)
#define PC4 GET_PIN(C, 4)
#define PC5 GET_PIN(C, 5)

#define THREAD_PRIORITY   25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE  5

static rt_thread_t tid = RT_NULL;

static rt_slist_t   _module_list;
static fro_module_t current_module = RT_NULL;

const char *fro_module_name     = RT_NULL;
char *      fro_module_info_str = RT_NULL;

static uint8_t get_fro_module_type(void)
{
    uint8_t module_type;
    uint8_t tmp;
    uint8_t retry = 3;

    do {
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

        if (tmp == module_type) {
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
    rt_slist_t * module_list    = RT_NULL;
    fro_module_t current_module = RT_NULL;
    uint8_t      type;

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

int fro_module_collect_data(uint8_t *              protocol_buf,
                            protocol_data_node_t **ptr_ptr_node)
{
    int      ret      = 0;
    uint8_t  reg_addr = protocol_buf[3];
    uint32_t cnt      = protocol_buf[5] << 1;
    if (current_module == RT_NULL) {
        return -1;
    }

    if (current_module->addr != reg_addr) {
        return -1;
    }

    if (current_module->ops->read == RT_NULL) {
        return -1;
    }
    ret = current_module->ops->read((void *)cnt, ptr_ptr_node);
    return ret;
}

int fro_module_event_process(uint8_t *protocol_buf)
{
    /* 该功能暂时未完成 */
    return 0;
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

static void module_test_thread_entry(void *parameter)
{
    fro_module_t         last_module = RT_NULL;
    struct rt_workqueue *wq          = RT_NULL;
    struct rt_work       work;
    uint32_t             work_status;

    wq = rt_workqueue_create("m_work", 2048, 24);
    if (wq == RT_NULL)
        return;

    for (;;) {
        current_module = get_current_module();
        if (current_module != RT_NULL) {
            if (last_module != current_module) {
                if (last_module != RT_NULL) {
                    // 处理识别错误的情况（偶尔接触不良会发生这种情况）
                    work_status = 0;
                    rt_workqueue_cancel_work_sync(wq, &work);
                    if (last_module->ops->deinit != RT_NULL)
                        last_module->ops->deinit();
                }

                if (current_module->ops->init != RT_NULL)
                    current_module->ops->init();
                rt_work_init(&work, current_module->ops->run, &work_status);
                work_status = 1;
                rt_workqueue_dowork(wq, &work);
            }
            // get module info
            if (current_module->ops->read != RT_NULL) {
                current_module->ops->read(0, &fro_module_info_str);
            }
            fro_module_name = current_module->name;
        } else {
            if (last_module != RT_NULL) {
                work_status = 0;
                rt_workqueue_cancel_work_sync(wq, &work);
                if (last_module->ops->deinit != RT_NULL)
                    last_module->ops->deinit();
            }
            fro_module_name     = "";
            fro_module_info_str = "";
        }
        last_module = current_module;

        rt_thread_mdelay(200);
    }
}

static int module_test_init(void)
{
    tid = rt_thread_create("m_test",
                           module_test_thread_entry,
                           RT_NULL,
                           THREAD_STACK_SIZE,
                           THREAD_PRIORITY,
                           THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(module_test_init);
