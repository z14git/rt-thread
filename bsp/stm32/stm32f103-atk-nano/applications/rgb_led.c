/**
 * @file rgb_led.c
 * @author z14git
 * @brief RGB灯模块
 * @version 0.1
 * @date 2019-11-12
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"

#define RGB_PWM_DEVICE "pwm2"
#define R_PWM_CH       1
#define G_PWM_CH       2
#define B_PWM_CH       3

#define B_LED_PIN GET_PIN(A, 2)
#define G_LED_PIN GET_PIN(A, 1)
#define R_LED_PIN GET_PIN(A, 0)

static const char *RED_ONOFF        = "R_SW";
static const char *GREEN_ONOFF      = "G_SW";
static const char *BLUE_ONOFF       = "B_SW";
static const char *RED_BRIGHTNESS   = "R_BV";
static const char *GREEN_BRIGHTNESS = "G_BV";
static const char *BLUE_BRIGHTNESS  = "B_BV";

static struct fro_module     rgb;
static struct rt_device_pwm *pwm_device = RT_NULL; //定义 pwm 设备指针
static int r_on_off, g_on_off, b_on_off, r_brightness, g_brightness,
    b_brightness;

extern void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

static TIM_HandleTypeDef htim2;

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{
    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_OC_InitTypeDef      oc_config          = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 0;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        goto __exit;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        goto __exit;
    }

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        goto __exit;
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) !=
        HAL_OK) {
        goto __exit;
    }

    oc_config.OCMode     = TIM_OCMODE_PWM1;
    oc_config.Pulse      = 0;
    oc_config.OCPolarity = TIM_OCPOLARITY_LOW;
    oc_config.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc_config, TIM_CHANNEL_1) !=
        HAL_OK) {
        goto __exit;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc_config, TIM_CHANNEL_2) !=
        HAL_OK) {
        goto __exit;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc_config, TIM_CHANNEL_3) !=
        HAL_OK) {
        goto __exit;
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    HAL_TIM_MspPostInit(&htim2);
    __HAL_TIM_URS_ENABLE(&htim2);
__exit:
    return;
    /* USER CODE END TIM2_Init 2 */
}

static int set_brightenss(uint8_t ch, uint8_t brightness)
{
    uint32_t period, pulse;
    /* 周期单位:ns */
    period = 1000000;
    /* 根据亮度计算脉冲宽度时间，LED灯低电平亮 */
    pulse = period / 100 * brightness;
    rt_pwm_set(pwm_device, ch, period, pulse);

    return 0;
}

static inline void r_on(void)
{
    set_brightenss(R_PWM_CH, r_brightness);
    r_on_off = 1;
}

static inline void g_on(void)
{
    set_brightenss(G_PWM_CH, g_brightness);
    g_on_off = 1;
}

static inline void b_on(void)
{
    set_brightenss(B_PWM_CH, b_brightness);
    b_on_off = 1;
}

static inline void r_off(void)
{
    set_brightenss(R_PWM_CH, 0);
    r_on_off = 0;
}

static inline void g_off(void)
{
    set_brightenss(G_PWM_CH, 0);
    g_on_off = 0;
}

static inline void b_off(void)
{
    set_brightenss(B_PWM_CH, 0);
    b_on_off = 0;
}

static void rgb_off(void)
{
    rt_pwm_disable(pwm_device, R_PWM_CH);
    rt_pwm_disable(pwm_device, G_PWM_CH);
    rt_pwm_disable(pwm_device, B_PWM_CH);
    r_on_off = 0;
    b_on_off = 0;
    g_on_off = 0;
}

static int rgb_init(void)
{
    MX_TIM2_Init();
    r_brightness = 100;
    g_brightness = 100;
    b_brightness = 100;
    pwm_device   = (struct rt_device_pwm *)rt_device_find(RGB_PWM_DEVICE);
    rt_pwm_enable(pwm_device, R_PWM_CH);
    rt_pwm_enable(pwm_device, B_PWM_CH);
    rt_pwm_enable(pwm_device, G_PWM_CH);
    r_on();
    b_on();
    g_on();
    return 0;
}

static void rgb_run(struct rt_work *work, void *param)
{
}

static void rgb_deinit(void)
{
    rgb_off();
    rt_pin_mode(B_LED_PIN, PIN_MODE_INPUT);
    rt_pin_mode(G_LED_PIN, PIN_MODE_INPUT);
    rt_pin_mode(R_LED_PIN, PIN_MODE_INPUT);
}

static int rgb_write(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, RED_ONOFF) == 0) {
            if ((int)data == 1) {
                r_on();
            } else if ((int)data == 0) {
                r_off();
            } else {
                return -1;
            }
            return 0;
        }
        if (rt_strcmp((char *)cmd, GREEN_ONOFF) == 0) {
            if ((int)data == 1) {
                g_on();
            } else if ((int)data == 0) {
                g_off();
            } else {
                return -1;
            }
            return 0;
        }
        if (rt_strcmp((char *)cmd, BLUE_ONOFF) == 0) {
            if ((int)data == 1) {
                b_on();
            } else if ((int)data == 0) {
                b_off();
            } else {
                return -1;
            }
            return 0;
        }
        if (rt_strcmp((char *)cmd, RED_BRIGHTNESS) == 0) {
            if ((int)data >= 0 && (int)data <= 100) {
                r_brightness = (int)data;
                if (r_on_off)
                    set_brightenss(R_PWM_CH, r_brightness);
            } else {
                return -1;
            }
            return 0;
        }
        if (rt_strcmp((char *)cmd, GREEN_BRIGHTNESS) == 0) {
            if ((int)data >= 0 && (int)data <= 100) {
                g_brightness = (int)data;
                if (g_on_off)
                    set_brightenss(G_PWM_CH, g_brightness);
            } else {
                return -1;
            }
            return 0;
        }
        if (rt_strcmp((char *)cmd, BLUE_BRIGHTNESS) == 0) {
            if ((int)data >= 0 && (int)data <= 100) {
                b_brightness = (int)data;
                if (b_on_off)
                    set_brightenss(B_PWM_CH, b_brightness);
            } else {
                return -1;
            }
            return 0;
        }
        return -1;
    }
    return 0;
}

static int rgb_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, RED_ONOFF) == 0) {
            *(double *)data = r_on_off;
            return 0;
        }
        if (rt_strcmp((char *)cmd, GREEN_ONOFF) == 0) {
            *(double *)data = g_on_off;
            return 0;
        }
        if (rt_strcmp((char *)cmd, BLUE_ONOFF) == 0) {
            *(double *)data = b_on_off;
            return 0;
        }
        if (rt_strcmp((char *)cmd, RED_BRIGHTNESS) == 0) {
            *(double *)data = r_brightness;
            return 0;
        }
        if (rt_strcmp((char *)cmd, GREEN_BRIGHTNESS) == 0) {
            *(double *)data = g_brightness;
            return 0;
        }
        if (rt_strcmp((char *)cmd, BLUE_BRIGHTNESS) == 0) {
            *(double *)data = b_brightness;
            return 0;
        }
        return -1;
    }

    return -1;
}

static const struct fro_module_ops rgb_ops = {
    rgb_init,
    rgb_deinit,
    rgb_run,
    rgb_write,
    rgb_read,
};

static int rgb_module_init(void)
{
    rgb.ops  = &rgb_ops;
    rgb.type = M_RGB_LIGHT;
    rgb.name = "RGB灯模块";

    fro_module_register(&rgb);
    return 0;
}
INIT_COMPONENT_EXPORT(rgb_module_init);
