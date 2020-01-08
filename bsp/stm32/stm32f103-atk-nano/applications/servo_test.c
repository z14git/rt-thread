/**
 * @file servo_test.c
 * @author z14git
 * @brief 舵机测试程序，生产用
 * @version 0.1
 * @date 2020-01-06
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"
#include "pca9685.h"

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "SERVO"
    #define LOG_LVL LOG_LVL_INFO
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define I2C_BUS "i2c1"

#define SDA_PIN GET_PIN(A, 0)
#define SCL_PIN GET_PIN(A, 1)

#define UP_SERVO   1
#define DOWN_SERVO 0
#define DUMP_SERVO 3

static struct fro_module servo;
static pca9685_device_t  dev = RT_NULL;

static const char *CMD_UP_SERVO_DEGREE   = "舵机上";
static const char *CMD_DOWN_SERVO_DEGREE = "舵机下";
static const char *CMD_AUTO_MODE         = "自动模式";
static const char *CMD_RESET             = "reset";

volatile static int     up_degree = 0, down_degree = 0, auto_mode = 0;
volatile static uint8_t first_flag  = 0;
volatile static uint8_t update_flag = 0;

/**
 * @brief 设置舵机角度
 * 
 * @param servo_num 舵机对应的PWM号
 * @param angle 舵机角度
 */
static void set_servo_angle(uint8_t servo_num, int angle)
{
    /**
     * 舵机角度 - end_time数值 - 高电平时间对应关系：
     * 0℃ -- 100 -- 0.5ms
     * 180℃ -- 500 -- 2.5ms
     * 
     * 该对应关系的由来：
     * 50hz * 0.982(pca9685的驱动中使用了这个常数) = 49.1hz ≈ 49
     * 0.5ms / (1s/49) * 4096 = 100.352 ≈ 100
     * 2.5ms / (1s/49) * 4096 = 501.76 ≈ 500
     */

    RT_ASSERT(dev);
    uint16_t end_time;
    end_time = angle * (500 - 100) / 180 + 100;
    pca9685_set_pwm(dev, servo_num, 0, end_time);
}

static int servo_init(void)
{
    rt_pin_mode(SDA_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(SCL_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(SDA_PIN, PIN_HIGH);
    rt_pin_write(SCL_PIN, PIN_HIGH);

    dev = pca9685_init(I2C_BUS, 0x60);

    RT_ASSERT(dev);
    pca9685_set_pwm_freq(dev, 50);

    return 0;
}

static void servo_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    int8_t count;
    LOG_I("enter servo run");
    for (;;) {
        /*
         * FIXME: 目前测试发现，首次设置完舵机角度后，需要等一段时间，
         * 再重复设置多次才正常
         */
        if (update_flag) {
            update_flag = 0;
            count       = 10;
        }
        if (count > 0) {
            count--;
            set_servo_angle(DUMP_SERVO, 90);
        }
        rt_thread_mdelay(100);
        if (*ptr_work_status == 0)
            break;
    }
}

static int servo_write(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, CMD_UP_SERVO_DEGREE) == 0) {
            if ((int)data >= 0 && (int)data <= 180) {
                up_degree = (int)data;
                set_servo_angle(UP_SERVO, up_degree);
                /*
                 * FIXME: 目前测试发现，首次设置完舵机角度后，需要等一段时间，
                 * 再重复设置多次才正常
                 */
                if (first_flag == 0) {
                    first_flag  = 1;
                    update_flag = 1;
                }
            } else {
                return -1;
            }
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_DOWN_SERVO_DEGREE) == 0) {
            if ((int)data >= 0 && (int)data <= 180) {
                down_degree = (int)data;
                set_servo_angle(DOWN_SERVO, down_degree);
                /*
                 * FIXME: 目前测试发现，首次设置完舵机角度后，需要等一段时间，
                 * 再重复设置多次才正常
                 */
                if (first_flag == 0) {
                    update_flag = 1;
                    first_flag  = 1;
                }
            } else {
                return -1;
            }
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_AUTO_MODE) == 0) {
            if ((int)data == 1) {
                auto_mode = 1;
            } else if ((int)data == 0) {
                auto_mode = 0;
            } else {
                return -1;
            }
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_RESET) == 0) {
            pca9685_restart(dev);
            return 0;
        }

        return -1;
    }
    return 0;
}

static int servo_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, CMD_UP_SERVO_DEGREE) == 0) {
            *(double *)data = up_degree;
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_DOWN_SERVO_DEGREE) == 0) {
            *(double *)data = down_degree;
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_AUTO_MODE) == 0) {
            *(double *)data = auto_mode;
            return 0;
        }
        return -1;
    }
    return -1;
}

static void servo_deinit(void)
{
    rt_pin_mode(SCL_PIN, PIN_MODE_INPUT);
    rt_pin_mode(SDA_PIN, PIN_MODE_INPUT);
}

static const struct fro_module_ops servo_ops = {
    servo_init,
    servo_deinit,
    servo_run,
    servo_write,
    servo_read,
};

static int servo_module_init(void)
{
    servo.ops  = &servo_ops;
    servo.type = M_SERVO_TEST;
    servo.name = "舵机调试";

    fro_module_register(&servo);
    return 0;
}
INIT_COMPONENT_EXPORT(servo_module_init);
