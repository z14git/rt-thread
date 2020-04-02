/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-8      zylx         first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
#include <board.h>

#define NOR_FLASH_DEV_NAME "W25Q128"

// extern const struct fal_flash_dev stm32_onchip_flash;
extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                                    \
    {                                                                          \
        &nor_flash0,                                                           \
    }
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG

/* partition table */
#define FAL_PART_TABLE                                                         \
    {                                                                          \
        {FAL_PART_MAGIC_WROD,                                                  \
         "app",                                                                \
         NOR_FLASH_DEV_NAME,                                                   \
         8 * 1024 * 1024,                                                      \
         14 * 1024 * 1024,                                                     \
         0},                                                                   \
            {FAL_PART_MAGIC_WROD,                                              \
             "log",                                                            \
             NOR_FLASH_DEV_NAME,                                               \
             14 * 1024 * 1024,                                                 \
             16 * 1024 * 1024,                                                 \
             0},                                                               \
    }
#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
