/**
 * @file mouse.c
 * @author z14git
 * @brief
 * @version 0.1
 * @date 2019-09-12
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "MOUSE"
    #define LOG_LVL LOG_LVL_DBG
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define THREAD_PRIORITY   25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE  5

#define CURSOR_STEP 4

static rt_thread_t tid = RT_NULL;

static uint8_t HID_Buffer[4];

/**
 * @brief  Gets Pointer Data.
 * @param  pbuf: Pointer to report
 * @retval None
 */
static void GetPointerData(uint8_t *pbuf)
{
    static int8_t cnt = 0;
    int8_t        x = 0, y = 0;

    if (cnt++ > 0) {
        x = CURSOR_STEP;
    } else {
        x = -CURSOR_STEP;
    }
    cnt++;
    pbuf[0] = 0;
    pbuf[1] = x;
    pbuf[2] = y;
    pbuf[3] = 0;
}

/* 线程1的入口函数 */
static void thread_entry(void *parameter)
{
    rt_device_t device = RT_NULL;

    device = rt_device_find("hidd");
    if (device == RT_NULL) {
        LOG_D("no hidd device");
        return;
    } else {
        rt_device_open(device, RT_DEVICE_FLAG_WRONLY);
    }

    while (1) {
        rt_thread_mdelay(40);
        GetPointerData(HID_Buffer);
        rt_device_write(device, HID_REPORT_ID_MOUSE, HID_Buffer, 4);
    }
}

int mouse_init(void)
{
    tid = rt_thread_create("mouse",
                           thread_entry,
                           RT_NULL,
                           THREAD_STACK_SIZE,
                           THREAD_PRIORITY,
                           THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(mouse_init);
