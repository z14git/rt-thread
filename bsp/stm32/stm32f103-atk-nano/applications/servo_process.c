/**
 * @file servo_process.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-23
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "zl_servo.h"

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "SERVO_P"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#include <stdlib.h> /* strtol */

#define ADAPTER_SERVO_MSG_NUM 10

#define ADAPTER_SERVO_THREAD_STACK_SIZE 2048

static struct zl_servo_device *up_servo = RT_NULL, *down_servo = RT_NULL;

static void adapter_servo_handler(void *arg)
{
        up_servo = (struct zl_servo_device *)rt_device_find("up_servo");
        down_servo = (struct zl_servo_device *)rt_device_find("down_servo");
        if (RT_NULL == up_servo) {
                LOG_E("no up_servo device");
        }
        if (RT_NULL == down_servo) {
                LOG_E("no down_servo device");
        }
        up_servo->ops->enable(up_servo, 1);
        down_servo->ops->enable(down_servo, 1);
        for (;;) {
                rt_thread_delay(10);
        }
}

static int adapter_servo_start(void)
{
        rt_thread_t tid;
        tid = rt_thread_create("servo_thr", adapter_servo_handler, RT_NULL,
                               ADAPTER_SERVO_THREAD_STACK_SIZE,
                               RT_THREAD_PRIORITY_MAX / 2, 50);
        if (tid != RT_NULL) {
                rt_thread_startup(tid);
        }
        return 0;
}
INIT_DEVICE_EXPORT(adapter_servo_start);

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)
static int servo_control(int argc, char **argv)
{
        char *p_end;
        if (argc == 3) {
                int ch = strtol(argv[1], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[1]);
                        return -1;
                }
                int pos = strtol(argv[2], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[2]);
                        return -1;
                }
                if (ch == 1) {
                        up_servo->ops->set_pos(up_servo, pos);
                }
                if (ch == 2) {
                        down_servo->ops->set_pos(down_servo, pos);
                }
        }
        return 0;
}
MSH_CMD_EXPORT(servo_control, channel pos);

static int servo_enable(int argc, char **argv)
{
        char *p_end;
        if (argc == 3) {
                int ch = strtol(argv[1], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[1]);
                        return -1;
                }
                int en = strtol(argv[2], &p_end, 0);
                if (*p_end != '\0') {
                        LOG_W("illegal parameter format: '%s'\r\n", argv[2]);
                        return -1;
                }
                if (ch == 1) {
                        up_servo->ops->enable(up_servo, en);
                }
                if (ch == 2) {
                        down_servo->ops->enable(down_servo, en);
                }
        }
        return 0;
}
MSH_CMD_EXPORT(servo_enable, channel enable);
#endif /* defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) */
