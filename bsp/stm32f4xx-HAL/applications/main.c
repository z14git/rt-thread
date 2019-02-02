/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-07-29     Arda.Fu      first implementation
 */
#include <rtthread.h>
#include <board.h>
#include <dfs_fs.h>
#include "dfs_romfs.h"
#include "finsh.h"

int main(void)
{
    /* user app entry */
    if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) == 0)
    {
        rt_kprintf("ROM file system initializated!\n");
    }
    else
    {
        rt_kprintf("ROM file system initializate failed!\n");
    }
    return 0;
}

int mw25(int argc, char **argv)
{
    if (dfs_mount("flash0", "/w25", "elm", 0, 0) == 0)
    {
        rt_kprintf("flash0 mount to /w25.\n");
    }
    else
    {
        rt_kprintf("flash0 mount to /w25 failed.\n");
    }

    return 0;
}
MSH_CMD_EXPORT(mw25, mount w25);
