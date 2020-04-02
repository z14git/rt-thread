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
#include <fal.h>

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "main"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

/* 定义要使用的分区名字 */
#define FS_PARTITION_NAME "app"

int main(void)
{
    struct rt_device *mtd_dev = RT_NULL;
    /* 初始化 fal */
    fal_init();
    /* 生成 mtd 设备 */
    mtd_dev = fal_mtd_nor_device_create(FS_PARTITION_NAME);
    if (!mtd_dev) {
        LOG_E("Can't create a mtd device on '%s' partition.",
              FS_PARTITION_NAME);
    } else {
        /* 挂载 littlefs */
        if (dfs_mount(FS_PARTITION_NAME, "/", "lfs", 0, 0) == 0) {
            LOG_I("Filesystem initialized!");
        } else {
            /* 格式化文件系统 */
            dfs_mkfs("lfs", FS_PARTITION_NAME);
            /* 挂载 littlefs */
            if (dfs_mount("filesystem", "/", "lfs", 0, 0) == 0) {
                LOG_I("Filesystem initialized!");
            } else {
                LOG_E("Failed to initialize filesystem!");
            }
        }
    }

    return RT_EOK;
}
