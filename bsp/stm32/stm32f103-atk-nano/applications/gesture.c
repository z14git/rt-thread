/**
 * @file gesture.c
 * @author z14git
 * @brief 手势传感器
 * @version 0.1
 * @date 2019-11-13
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"
#include "APDS9960.h"

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "GESTURE"
    #define LOG_LVL LOG_LVL_ASSERT
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define SDA_PIN GET_PIN(A, 0)
#define SCL_PIN GET_PIN(A, 1)
#define INT_PIN GET_PIN(A, 2)

static struct fro_module gesture;
static rt_sem_t          sid = RT_NULL;

static void isr_int_cb(void *args)
{
    rt_sem_release(sid);
}

static int gesture_init(void)
{
    sid = rt_sem_create("int", 0, RT_IPC_FLAG_FIFO);
    if (sid == RT_NULL) {
        LOG_D("create semaphore failed");
        return -1;
    }
    rt_pin_mode(SDA_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(SCL_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(SDA_PIN, PIN_HIGH);
    rt_pin_write(SCL_PIN, PIN_HIGH);
    rt_pin_mode(INT_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(INT_PIN, PIN_IRQ_MODE_FALLING, isr_int_cb, RT_NULL);
    rt_pin_irq_enable(INT_PIN, PIN_IRQ_ENABLE);
    initialize();
    enableGestureSensor(1);
    rt_thread_mdelay(100);
    return 0;
}

static void gesture_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    uint8_t  gesture;
    rt_err_t res;

    for (;;) {
        res = rt_sem_take(sid, 1500);
        if (res == RT_EOK) {
            if (isGestureAvailable()) {
                gesture = readGesture();

                if (gesture == DIR_FAR) {
                    fro_module_info_str = "FAR";
                    LOG_D("       F A R        ");
                }

                if (gesture == DIR_RIGHT) {
                    fro_module_info_str = "RIGHT";
                    LOG_D("     R I G H T      ");
                }

                if (gesture == DIR_LEFT) {
                    fro_module_info_str = "LEFT";
                    LOG_D("      L E F T       ");
                }

                if (gesture == DIR_UP) {
                    fro_module_info_str = "UP";
                    LOG_D("        U P         ");
                }

                if (gesture == DIR_DOWN) {
                    fro_module_info_str = "DOWN";
                    LOG_D("      D O W N       ");
                }

                if (gesture == DIR_NEAR) {
                    fro_module_info_str = "NEAR";
                    LOG_D("      N E A R       ");
                }
            }
        } else {
            fro_module_info_str = "";
        }
        if (*ptr_work_status == 0)
            break;
    }
}

static void gesture_deinit(void)
{
    rt_pin_mode(SDA_PIN, PIN_MODE_INPUT);
    rt_pin_mode(SCL_PIN, PIN_MODE_INPUT);
    rt_pin_irq_enable(INT_PIN, PIN_IRQ_DISABLE);
    rt_pin_detach_irq(INT_PIN);
    rt_pin_mode(INT_PIN, PIN_MODE_INPUT);
    if (sid != RT_NULL) {
        rt_sem_delete(sid);
        sid = RT_NULL;
    }
}

static const struct fro_module_ops gesture_ops = {
    gesture_init,
    gesture_deinit,
    gesture_run,
    RT_NULL,
    RT_NULL,
};

static int gesture_module_init(void)
{
    gesture.ops  = &gesture_ops;
    gesture.type = M_GESTURE;
    gesture.name = "手势传感器";

    fro_module_register(&gesture);
    return 0;
}
INIT_COMPONENT_EXPORT(gesture_module_init);
