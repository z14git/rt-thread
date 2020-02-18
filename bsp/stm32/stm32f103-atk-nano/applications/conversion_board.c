/**
 * @file conversion_board.c
 * @author z14git
 * @brief 转接板测试程序，生产用
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
#include "drv_pca9685.h"
#include "drv_motor.h"
#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "SERVO"
    #define LOG_LVL LOG_LVL_ASSERT
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define I2C_BUS "i2c1"

#define SDA_PIN GET_PIN(A, 0)
#define SCL_PIN GET_PIN(A, 1)

#define UP_SERVO   1
#define DOWN_SERVO 0

#define AIN1 10
#define AIN2 9
#define PWMA 8
#define BIN1 11
#define BIN2 12
#define PWMB 13

static struct fro_module    servo;
static pca9685_device_t     dev    = RT_NULL;
static struct tb6612_motor *lmotor = RT_NULL;
static struct tb6612_motor *rmotor = RT_NULL;

static const char *CMD_UP_SERVO_DEGREE   = "舵机上";
static const char *CMD_DOWN_SERVO_DEGREE = "舵机下";
static const char *CMD_AUTO_MODE         = "自动模式";
static const char *CMD_LEFT_MOTOR        = "电机左";
static const char *CMD_RIGHT_MOTOR       = "电机右";

volatile static int up_degree = -1, down_degree = -1, auto_mode = 0;

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
     * 0℃ -- 102 -- 0.5ms
     * 180℃ -- 512 -- 2.5ms
     * 
     * 该对应关系的由来：
     * 0.5ms / (1s/50hz) * 4096 = 102.4 ≈ 102
     * 2.5ms / (1s/50hz) * 4096 = 512
     */

    RT_ASSERT(dev);
    if (angle < 0 || angle > 180) {
        return;
    }
    uint16_t end_time;
    end_time = angle * (512 - 102) / 180 + 102;
    dev->ops->set_pwm(dev, servo_num, 0, end_time);
}

static int servo_init(void)
{
    static struct tb6612_pin pin1, pin2;
    pin1.in1 = AIN1;
    pin1.in2 = AIN2;
    pin1.pwm = PWMA;
    pin2.in1 = BIN1;
    pin2.in2 = BIN2;
    pin2.pwm = PWMB;

    rt_pin_mode(SDA_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(SCL_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(SDA_PIN, PIN_HIGH);
    rt_pin_write(SCL_PIN, PIN_HIGH);

    dev = pca9685_device_create(I2C_BUS, 0x60);

    RT_ASSERT(dev);
    dev->ops->init(dev);
    dev->ops->set_pwm_freq(dev, 50);

    lmotor = create_motor_device(&pin2, dev);
    rmotor = create_motor_device(&pin1, dev);
    return 0;
}

static void servo_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    LOG_I("enter servo run");
    for (;;) {
        if (auto_mode) {
            for (uint8_t i = 0; i < 180; i++) {
                set_servo_angle(UP_SERVO, i);
                set_servo_angle(DOWN_SERVO, i);
                if (!auto_mode)
                    break;
                rt_thread_mdelay(20);
                if (*ptr_work_status == 0)
                    break;
            }
            for (int16_t i = 180; i > 0; i--) {
                set_servo_angle(UP_SERVO, i);
                set_servo_angle(DOWN_SERVO, i);
                if (!auto_mode)
                    break;
                rt_thread_mdelay(20);
                if (*ptr_work_status == 0)
                    break;
            }
        } else {
            set_servo_angle(UP_SERVO, up_degree);
            set_servo_angle(DOWN_SERVO, down_degree);
        }
        rt_thread_mdelay(300); // BUG?:若间隔太小舵机可能不会转
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
            } else {
                return -1;
            }
            auto_mode = 0;
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_DOWN_SERVO_DEGREE) == 0) {
            if ((int)data >= 0 && (int)data <= 180) {
                down_degree = (int)data;
                set_servo_angle(DOWN_SERVO, down_degree);
            } else {
                return -1;
            }
            auto_mode = 0;
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

        if (rt_strcmp((char *)cmd, CMD_LEFT_MOTOR) == 0) {
            lmotor->ops->set_throttle(lmotor, (int)data);
            return 0;
        }

        if (rt_strcmp((char *)cmd, CMD_RIGHT_MOTOR) == 0) {
            rmotor->ops->set_throttle(rmotor, (int)data);
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
    //TODO: delete all pca9685_device_t and struct tb6612_motor * object
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
