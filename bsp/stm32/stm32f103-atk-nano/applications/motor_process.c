/**
 * @file motor_process.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "zl_motor.h"

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "SERVO_P"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#include <stdlib.h> /* strtol */

#define ADAPTER_MOTOR_THREAD_STACK_SIZE 2048

static struct zl_motor_device *left_motor = RT_NULL, *right_motor = RT_NULL;

static void adapter_motor_handler(void *arg)
{
        left_motor = (struct zl_motor_device *)rt_device_find("left");
        right_motor = (struct zl_motor_device *)rt_device_find("right");

        if (RT_NULL == left_motor) {
                LOG_E("no left_motor device");
                return;
        }
        if (RT_NULL == right_motor) {
                LOG_E("no right_motor device");
                return;
        }

        left_motor->ops->enable(left_motor, 1);
        right_motor->ops->enable(right_motor, 1);
        for (;;) {
                rt_thread_delay(10);
        }
}

static int adapter_motor_start(void)
{
        rt_thread_t tid;
        tid = rt_thread_create("motor_thr", adapter_motor_handler, RT_NULL,
                               ADAPTER_MOTOR_THREAD_STACK_SIZE,
                               RT_THREAD_PRIORITY_MAX / 2, 50);
        if (tid != RT_NULL) {
                rt_thread_startup(tid);
        }
        return 0;
}
INIT_DEVICE_EXPORT(adapter_motor_start);

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)

static int motor_control(int argc, char **argv)
{
        char *p_end;
        if (argc == 3) {
                int id = strtol(argv[1], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[1]);
                        return -1;
                }
                int speed = strtol(argv[2], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[2]);
                        return -1;
                }
                if (id == 1) {
                        left_motor->ops->set_throttle(left_motor, speed);
                }
                if (id == 2) {
                        right_motor->ops->set_throttle(right_motor, speed);
                }
        }
        return 0;
}
MSH_CMD_EXPORT(motor_control, id speed);
#endif /* defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) */
