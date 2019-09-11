/**
 * @file button.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-09-11
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "button.h"

#define LED1_PIN GET_PIN(C, 13)
#define BEEP_PIN GET_PIN(B, 2)

#define BUTTON1_PIN GET_PIN(C, 10)
#define BUTTON2_PIN GET_PIN(C, 11)

static rt_timer_t timer;

Button_t Button1;
Button_t Button2;

static void timeout(void *parameter)
{
    Button_Process();
}

static rt_uint8_t get_button1_level(void)
{
    return rt_pin_read(BUTTON1_PIN);
}

static rt_uint8_t get_button2_level(void)
{
    return rt_pin_read(BUTTON2_PIN);
}

static void btn_down_cb(void *btn)
{
    static uint8_t led_state = 0;
    static uint8_t beep_state = 0;

    if (btn == &Button1)
    {
        led_state ^= 0xff;
        if (led_state)
        {
            rt_pin_write(LED1_PIN, PIN_HIGH);
        }
        else
        {
            rt_pin_write(LED1_PIN, PIN_LOW);
        }
        rt_kprintf("button 1 down\n");
    }
    if (btn == &Button2)
    {
        beep_state ^= 0xff;
        if (beep_state)
        {
            rt_pin_write(BEEP_PIN, PIN_HIGH);
        }
        else
        {
            rt_pin_write(BEEP_PIN, PIN_LOW);
        }
        rt_kprintf("button 2 down\n");
    }
}

static int button_init(void)
{
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);

    rt_pin_mode(BUTTON1_PIN, PIN_MODE_INPUT);
    rt_pin_mode(BUTTON2_PIN, PIN_MODE_INPUT);

    Button_Create("Button1",
                  &Button1,
                  get_button1_level,
                  PIN_LOW);

    Button_Create("Button2",
                  &Button2,
                  get_button2_level,
                  PIN_LOW);

    Button_Attach(&Button1, BUTTON_DOWM, btn_down_cb);
    Button_Attach(&Button2, BUTTON_DOWM, btn_down_cb);

    timer = rt_timer_create("button_scan", timeout, RT_NULL, 20, RT_TIMER_FLAG_PERIODIC);
    if (timer != RT_NULL)
    {
        rt_timer_start(timer);
    }
    return 0;
}
INIT_APP_EXPORT(button_init);
