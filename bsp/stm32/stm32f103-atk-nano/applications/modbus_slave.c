/**
 * @file modbus_slave.c
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

#include "modbus.h"

#include "servo_process.h"
#include "motor_process.h"

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "MB_SLAVE"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define MODBUS_DEVICE "/dev/uart2"

#define SLAVE_ADDR 14
#define PORT_BAUDRATE 115200

#define THREAD_STACK_SIZE 2048
#define THRAD_PRIORITY 15

typedef enum {
        ADAPTER_MODBUS_CONFIG_SERVO_ON,
        ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE,
        ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE,
        ADAPTER_MODBUS_CONFIG_LEFT_PIE_TIMEOUT,
        ADAPTER_MODBUS_CONFIG_RIGHT_PIE_TIMEOUT,
        ADAPTER_MODBUS_CONFIG_LEFT_MOTOR_SPEED,
        ADAPTER_MODBUS_CONFIG_RIGHT_MOTOR_SPEED,
        ADAPTER_MODBUS_CONFIG_MAX
} adapter_modbus_config_t;

typedef enum {
        /* 拔片动作，动作执行完成后自动归0 */
        ADAPTER_MODBUS_BIT_LEFT_BLOCK, // combine block and pass bit, 10(block:1, pass:0): do block;
        ADAPTER_MODBUS_BIT_LEFT_PASS, // 01: do pass; 11: auto
        ADAPTER_MODBUS_BIT_RIGHT_BLOCK,
        ADAPTER_MODBUS_BIT_RIGHT_PASS,
        /* 限位开关状态 */
        ADAPTER_MODBUS_BIT_LEFT_UP,
        ADAPTER_MODBUS_BIT_LEFT_DOWN,
        ADAPTER_MODBUS_BIT_RIGHT_UP,
        ADAPTER_MODBUS_BIT_RIGHT_DOWN,
        ADAPTER_MODBUS_BIT_MAX
} adapter_modbus_bit_t;

static modbus_mapping_t *mb_mapping = NULL;

/**
 * @brief 用于保存上一次的HOLD REG值，以便检查哪些HOLD REG的值更新了
 * 
 */
static uint8_t modbus_config_buffer[ADAPTER_MODBUS_CONFIG_MAX];

/**
 * @brief 检测哪些HOLD REG 或 coil 被修改，根据其改变，发送相应的信息给其他线程处理
 * 
 * @param old_state 保存上一次HOLD REG状态的数组
 * @param mb_mapping 
 */
static void hold_reg_process(uint8_t *old_state, modbus_mapping_t *mb_mapping)
{
        struct adapter_servo_msg servo_msg;
        struct adapter_motor_msg motor_msg;

#define IS_HOLD_REG_UPDATED(x)                                                 \
        !(old_state[x] == (mb_mapping->tab_registers[x] & 0xff))

#define SAVE_HOLD_REG(x)                                                       \
        do {                                                                   \
                old_state[x] = mb_mapping->tab_registers[x];                   \
        } while (0)

        /* handle servo */
        if (IS_HOLD_REG_UPDATED(ADAPTER_MODBUS_CONFIG_SERVO_ON)) {
                servo_msg.cmd = ADAPTER_SERVO_CMD_ON_OFF;
                servo_msg.data =
                        mb_mapping
                                ->tab_registers[ADAPTER_MODBUS_CONFIG_SERVO_ON];
                adapter_servo_msg_put(&servo_msg);
                SAVE_HOLD_REG(ADAPTER_MODBUS_CONFIG_SERVO_ON);
        }

        if (IS_HOLD_REG_UPDATED(ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE)) {
                servo_msg.cmd = ADAPTER_SERVO_CMD_UP_ANGLE;
                servo_msg.data = mb_mapping->tab_registers
                                         [ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE];
                adapter_servo_msg_put(&servo_msg);
                SAVE_HOLD_REG(ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE);
        }

        if (IS_HOLD_REG_UPDATED(ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE)) {
                servo_msg.cmd = ADAPTER_SERVO_CMD_DOWN_ANGLE;
                servo_msg.data =
                        mb_mapping->tab_registers
                                [ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE];
                adapter_servo_msg_put(&servo_msg);
                SAVE_HOLD_REG(ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE);
        }

        /* handle motor */
        if (IS_HOLD_REG_UPDATED(ADAPTER_MODBUS_CONFIG_LEFT_MOTOR_SPEED)) {
                motor_msg.cmd = ADAPTER_MOTOR_CMD_LEFT_SPEED;
                motor_msg.data =
                        mb_mapping->tab_registers
                                [ADAPTER_MODBUS_CONFIG_LEFT_MOTOR_SPEED];
                adapter_motor_msg_put(&motor_msg);
                SAVE_HOLD_REG(ADAPTER_MODBUS_CONFIG_LEFT_MOTOR_SPEED);
        }

        if (IS_HOLD_REG_UPDATED(ADAPTER_MODBUS_CONFIG_RIGHT_MOTOR_SPEED)) {
                motor_msg.cmd = ADAPTER_MOTOR_CMD_RIGHT_SPEED;
                motor_msg.data =
                        mb_mapping->tab_registers
                                [ADAPTER_MODBUS_CONFIG_RIGHT_MOTOR_SPEED];
                adapter_motor_msg_put(&motor_msg);
                SAVE_HOLD_REG(ADAPTER_MODBUS_CONFIG_RIGHT_MOTOR_SPEED);
        }
}

static void mb_slave_handler(void *arg)
{
        int rc;
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        modbus_t *ctx = RT_NULL;
        ctx = modbus_new_rtu(MODBUS_DEVICE, 115200, 'N', 8, 1);
        modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
        modbus_set_slave(ctx, SLAVE_ADDR);
        modbus_connect(ctx);
        modbus_set_response_timeout(ctx, 0, 1000000);
        mb_mapping = modbus_mapping_new(ADAPTER_MODBUS_BIT_MAX + 1, 0,
                                        ADAPTER_MODBUS_CONFIG_MAX + 1, 0);
        if (mb_mapping == NULL) {
                modbus_free(ctx);
                return;
        }

        mb_mapping->tab_bits[ADAPTER_MODBUS_BIT_LEFT_BLOCK] = 0;
        mb_mapping->tab_bits[ADAPTER_MODBUS_BIT_LEFT_PASS] = 0;
        mb_mapping->tab_bits[ADAPTER_MODBUS_BIT_RIGHT_BLOCK] = 0;
        mb_mapping->tab_bits[ADAPTER_MODBUS_BIT_RIGHT_PASS] = 0;

        while (1) {
                rc = modbus_receive(ctx, query);
                if (rc > 0) {
                        modbus_reply(ctx, query, rc, mb_mapping);
                        /* mb_mapping的值在modbus_reply()之后才会更新 */
                        hold_reg_process(modbus_config_buffer, mb_mapping);
                }
        }
}

static int mb_slave_start(void)
{
        rt_thread_t tid = RT_NULL;
        tid = rt_thread_create("mb_slave", mb_slave_handler, RT_NULL,
                               THREAD_STACK_SIZE, THRAD_PRIORITY, 20);
        if (tid != RT_NULL) {
                rt_thread_startup(tid);
                return 0;
        }
        return -1;
}
INIT_APP_EXPORT(mb_slave_start);
