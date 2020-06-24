/**
 * @file servo_process.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __SERVO_PROCESS_H
#define __SERVO_PROCESS_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        ADAPTER_SERVO_CMD_ON_OFF,
        ADAPTER_SERVO_CMD_UP_ANGLE,
        ADAPTER_SERVO_CMD_DOWN_ANGLE
} adapter_servo_cmd_t;

struct adapter_servo_msg {
        uint8_t cmd; ///> see adapter_servo_cmd_t
        uint16_t data;
};

rt_err_t adapter_servo_msg_put(struct adapter_servo_msg *msg);

#ifdef __cplusplus
}
#endif

#endif // __SERVO_PROCESS_H
