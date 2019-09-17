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
#include "oled.h"

/* defined the LED0 pin: PD2 */
#define LED0_PIN    GET_PIN(D, 2)

int main(void)
{
    int count = 1;
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    if (dfs_mount("W25Q128", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("W25Q128 mount to /.\n");
    }
    else
    {
        rt_kprintf("W25Q128 mount to / failed.\n");
    }

    while (count++)
    {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}

static int oled_write(int argc, char **argv)
{
    char *p_end;
    uint8_t x, y;
    uint8_t data;

    if (argc == 4)
    {
        x = strtol(argv[1], &p_end, 0);
        if (*p_end != '\0')
        {
            rt_kprintf("illegal x format: '%s'\r\n", argv[1]);
            return -1;
        }
        y = strtol(argv[2], &p_end, 0);
        if (*p_end != '\0')
        {
            rt_kprintf("illegal y format: '%s'\r\n", argv[1]);
            return -1;
        }
        data = strtol(argv[3], &p_end, 0);
        if (*p_end != '\0')
        {
            rt_kprintf("illegal data format: '%s'\r\n", argv[1]);
            return -1;
        }

        OLED_Set_Pos(x, y);
        OLED_WR_Byte(data, 1);
    }
    return 0;
}
MSH_CMD_EXPORT(oled_write, oled test);