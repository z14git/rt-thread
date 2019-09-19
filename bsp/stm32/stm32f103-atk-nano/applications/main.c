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

/* defined the LED0 pin: PD2 */
#define LED0_PIN GET_PIN(D, 2)
#define LED1_PIN GET_PIN(C, 13)

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
