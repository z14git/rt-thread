/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     balanceTWK   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <dfs_fs.h>
#include "fro_module.h"

/* defined the LED0 pin: PD2 */
#define LED0_PIN GET_PIN(D, 2)
#define LED1_PIN GET_PIN(C, 13)

#define THREAD_PRIORITY 25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE 5

static rt_thread_t tid = RT_NULL;

int main(void)
{
    int count = 1;
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);

    if (dfs_mount("W25Q128", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("W25Q128 mount to /.\n");
    }
    else
    {
        /* 格式化文件系统 */
        if (dfs_mkfs("elm", "W25Q128") != 0)
        {
            rt_kprintf("create file system failed\n");
        }
        rt_kprintf("create file system success\n");
        if (dfs_mount("W25Q128", "/", "elm", 0, 0) == 0)
        {
            rt_kprintf("W25Q128 mount to /.\n");
        }
        else
        {
            rt_kprintf("W25Q128 mount to / failed.\n");
        }
    }

    while (count++)
    {
        /* LED灯测试 */
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_pin_write(LED1_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_pin_write(LED1_PIN, PIN_LOW);
        rt_thread_mdelay(500);

        /* 每3秒发送一次'hola'，用于DB9测试 */
        if (count % 3 == 0)
        {
            rt_kprintf("hola\n");
        }
    }

    return RT_EOK;
}

static void module_test_thread_entry(void *parameter)
{
    fro_module_t current_module = RT_NULL;
    fro_module_t last_module = RT_NULL;
    struct rt_workqueue *wq = RT_NULL;
    struct rt_work work;

    wq = rt_workqueue_create("moduleWork", 1024, 25);
    if (wq == RT_NULL)
        return;

    for (;;)
    {
        current_module = get_current_module();
        if (current_module != RT_NULL)
        {
            if (last_module != current_module)
            {
                if (last_module->ops->init != RT_NULL)
                    current_module->ops->init();
                rt_work_init(&work, current_module->ops->run, RT_NULL);
                rt_workqueue_dowork(wq, &work);
            }
            // get module info
            if (current_module->ops->read != RT_NULL)
            {
                current_module->ops->read(0, &fro_module_info_str);
            }
            fro_module_name = current_module->name;
        }
        else
        {
            if (last_module != RT_NULL)
            {
                rt_workqueue_cancel_work(wq, &work);
                if (last_module->ops->deinit != RT_NULL)
                    last_module->ops->deinit();
            }
            fro_module_name = "";
            fro_module_info_str = "";
        }
        last_module = current_module;

        rt_thread_mdelay(200);
    }
}

static int module_test_init(void)
{

    tid = rt_thread_create("ModuleTest",
                            module_test_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(module_test_init);
