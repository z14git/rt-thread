/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 * 2018-11-19     flybreak     add stm32f429-fire-challenger bsp
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <dfs_fs.h>
#include "finsh.h"

/* defined the LED0 pin: PH10 */
#define LED0_PIN    GET_PIN(H, 10)

int main(void)
{
    int count = 1;
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    while (count++)
    {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}

int mw25(int argc, char **argv)
{
    if (dfs_mount("W25Q128", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("W25Q128 mount to /.\n");
    }
    else
    {
        rt_kprintf("W25Q128 mount to / failed.\n");
    }

    return 0;
}
MSH_CMD_EXPORT(mw25, mount w25);
