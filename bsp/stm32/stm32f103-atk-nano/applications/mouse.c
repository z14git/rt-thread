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
#include "usb_device.h"
#include "usbd_hid.h"

#define THREAD_PRIORITY 25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE 5

#define CURSOR_STEP 5

static rt_thread_t tid = RT_NULL;

extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t HID_Buffer[4];

/**
  * @brief  Gets Pointer Data.
  * @param  pbuf: Pointer to report
  * @retval None
  */
static void GetPointerData(uint8_t *pbuf)
{
    static int8_t cnt = 0;
    int8_t x = 0, y = 0;

    if (cnt++ > 0)
    {
        x = CURSOR_STEP;
    }
    else
    {
        x = -CURSOR_STEP;
    }

    pbuf[0] = 0;
    pbuf[1] = x;
    pbuf[2] = y;
    pbuf[3] = 0;
}

/* 线程1的入口函数 */
static void thread_entry(void *parameter)
{
    MX_USB_DEVICE_Init();
    while (1)
    {
        rt_thread_mdelay(100);
        GetPointerData(HID_Buffer);
        USBD_HID_SendReport(&hUsbDeviceFS, HID_Buffer, 4);
    }
}

int mouse_init(void)
{
    tid = rt_thread_create("mouse",
                           thread_entry, RT_NULL,
                           THREAD_STACK_SIZE,
                           THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(mouse_init);

extern PCD_HandleTypeDef hpcd_USB_FS;

/**
  * @brief This function handles USB low priority or CAN RX0 interrupts.
  */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    /* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 0 */
    rt_interrupt_enter();
    /* USER CODE END USB_LP_CAN1_RX0_IRQn 0 */
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
    /* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 1 */
    rt_interrupt_leave();
    /* USER CODE END USB_LP_CAN1_RX0_IRQn 1 */
}
