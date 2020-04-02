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
#include <fal.h>

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "main"
    #define LOG_LVL LOG_LVL_INFO
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

/* defined the LED0 pin: PD2 */
#define LED0_PIN GET_PIN(D, 2)
#define LED1_PIN GET_PIN(C, 13)

/* 定义要使用的分区名字 */
#define FS_PARTITION_NAME "app"

int main(void)
{
    extern void u8g2_init(void);
    extern int  board_test_init(void);
    extern int  module_info_init(void);
    extern int  module_test_init(void);

    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);

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

    u8g2_init();
    if (get_current_module() == RT_NULL) {
        board_test_init();
    } else {
        module_info_init();
        module_test_init();
    }
    while (1) {
        /* LED灯测试 */
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_pin_write(LED1_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_pin_write(LED1_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}
