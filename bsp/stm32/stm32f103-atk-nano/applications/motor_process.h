/**
 * @file motor_process.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-29
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __MOTOR_PROCESS_H
#define __MOTOR_PROCESS_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        ADAPTER_MOTOR_CMD_LEFT_SPEED,
        ADAPTER_MOTOR_CMD_RIGHT_SPEED,
} adapter_motor_cmd_t;

struct adapter_motor_msg {
        uint8_t cmd;
        uint16_t data;
};

rt_err_t adapter_motor_msg_put(struct adapter_motor_msg *msg);

#ifdef __cplusplus
}
#endif

#endif // __MOTOR_PROCESS_H
