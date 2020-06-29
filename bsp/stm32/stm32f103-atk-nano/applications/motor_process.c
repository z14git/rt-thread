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
#include "motor_process.h"

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

static struct rt_messagequeue motor_mq;
static struct adapter_motor_msg motor_msg[10];

static rt_err_t adapter_motor_msg_init(void)
{
        return rt_mq_init(&motor_mq, "motor_mq", &motor_msg[0],
                          sizeof(struct adapter_motor_msg), sizeof(motor_msg),
                          RT_IPC_FLAG_FIFO);
}

rt_err_t adapter_motor_msg_put(struct adapter_motor_msg *msg)
{
        rt_err_t rst = RT_EOK;
        rst = rt_mq_send(&motor_mq, msg, sizeof(struct adapter_motor_msg));
        if (rst == -RT_EFULL) {
                LOG_I("motor msg queue is full");
        }
        return rst;
}

rt_err_t adapter_motor_msg_get(struct adapter_motor_msg *msg, uint32_t timeout)
{
        rt_err_t rst = RT_EOK;
        rst = rt_mq_recv(&motor_mq, msg, sizeof(struct adapter_motor_msg),
                         timeout);
        if ((rst != RT_EOK) && (rst != -RT_ETIMEOUT)) {
                LOG_E("Failed to get msg, Errno: %d", rst);
        }
        return rst;
}

static rt_err_t adapter_motor_process(struct adapter_motor_msg *msg)
{
        rt_err_t rst = RT_EOK;
        if (msg->cmd == ADAPTER_MOTOR_CMD_LEFT_SPEED) {
                rst = left_motor->ops->set_throttle(left_motor, msg->data);
        }
        if (msg->cmd == ADAPTER_MOTOR_CMD_RIGHT_SPEED) {
                rst = right_motor->ops->set_throttle(right_motor, msg->data);
        }
        return rst;
}

static void adapter_motor_handler(void *arg)
{
        rt_err_t rst = RT_EOK;
        struct adapter_motor_msg msg;

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
                rst = adapter_motor_msg_get(&msg, RT_WAITING_FOREVER);
                if (rst == RT_EOK) {
                        adapter_motor_process(&msg);
                }
        }
}

static int adapter_motor_start(void)
{
        rt_thread_t tid;
        adapter_motor_msg_init();
        tid = rt_thread_create("motor_thr", adapter_motor_handler, RT_NULL,
                               ADAPTER_MOTOR_THREAD_STACK_SIZE,
                               RT_THREAD_PRIORITY_MAX / 2, 50);
        if (tid != RT_NULL) {
                rt_thread_startup(tid);
        }
        return 0;
}
INIT_COMPONENT_EXPORT(adapter_motor_start);

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
