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
#include "servo_process.h"

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

static struct rt_messagequeue servo_mq;
static struct adapter_servo_msg servo_msg[ADAPTER_SERVO_MSG_NUM];

static rt_err_t adapter_servo_msg_init(void)
{
        return rt_mq_init(&servo_mq, "servo_mq", &servo_msg[0],
                          sizeof(struct adapter_servo_msg), sizeof(servo_msg),
                          RT_IPC_FLAG_FIFO);
}

rt_err_t adapter_servo_msg_put(struct adapter_servo_msg *msg)
{
        rt_err_t rst = RT_EOK;
        rst = rt_mq_send(&servo_mq, msg, sizeof(struct adapter_servo_msg));
        if (rst == -RT_EFULL) {
                LOG_I("servo msg queue is full");
        }
        return rst;
}

rt_err_t adapter_servo_msg_get(struct adapter_servo_msg *msg, uint32_t timeout)
{
        rt_err_t rst = RT_EOK;
        rst = rt_mq_recv(&servo_mq, msg, sizeof(struct adapter_servo_msg),
                         timeout);
        if ((rst != RT_EOK) && (rst != -RT_ETIMEOUT)) {
                LOG_E("iotb event get failed! Errno: %d", rst);
        }
        return rst;
}

static rt_err_t adapter_servo_process(struct adapter_servo_msg *msg)
{
        rt_err_t rst = RT_EOK;
        if (msg->cmd == ADAPTER_SERVO_CMD_ON_OFF) {
                up_servo->ops->enable(up_servo, msg->data);
                down_servo->ops->enable(down_servo, msg->data);
        }
        if (msg->cmd == ADAPTER_SERVO_CMD_UP_ANGLE) {
                up_servo->ops->set_pos(up_servo, msg->data);
        }
        if (msg->cmd == ADAPTER_SERVO_CMD_DOWN_ANGLE) {
                down_servo->ops->set_pos(down_servo, msg->data);
        }
        return rst;
}

static void adapter_servo_handler(void *arg)
{
        rt_err_t rst = RT_EOK;
        struct adapter_servo_msg msg;

        up_servo = (struct zl_servo_device *)rt_device_find("up_servo");
        down_servo = (struct zl_servo_device *)rt_device_find("down_servo");
        if (RT_NULL == up_servo) {
                LOG_E("no up_servo device");
                return;
        }
        if (RT_NULL == down_servo) {
                LOG_E("no down_servo device");
                return;
        }
        up_servo->ops->enable(up_servo, 1);
        down_servo->ops->enable(down_servo, 1);

        for (;;) {
                rst = adapter_servo_msg_get(&msg, 100);
                if (rst == RT_EOK) {
                        adapter_servo_process(&msg);
                }
        }
}

static int adapter_servo_start(void)
{
        rt_thread_t tid;
        adapter_servo_msg_init();
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
                struct adapter_servo_msg msg;
                if (ch == 1) {
                        msg.cmd = ADAPTER_SERVO_CMD_UP_ANGLE;
                        msg.data = pos;
                        adapter_servo_msg_put(&msg);
                }
                if (ch == 2) {
                        msg.cmd = ADAPTER_SERVO_CMD_DOWN_ANGLE;
                        msg.data = pos;
                        adapter_servo_msg_put(&msg);
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
