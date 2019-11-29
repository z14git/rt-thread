/**
 * @file buzzer.c
 * @author z14git
 * @brief 蜂鸣器模块
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

#define PA0               GET_PIN(A, 0)
#define BUZZER_PWM_DEVICE "pwm2"
#define BUZZER_PWM_CH     1

static struct fro_module     buzzer;
static struct rt_device_pwm *pwm_device = RT_NULL; //定义 pwm 设备指针

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
    oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc_config.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc_config, TIM_CHANNEL_2) !=
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

static int buzzer_on(void)
{
    rt_pwm_enable(pwm_device, BUZZER_PWM_CH); //使能蜂鸣器对应的 PWM 通道
    return 0;
}

static int buzzer_off(void)
{
    rt_pwm_disable(pwm_device, BUZZER_PWM_CH); //失能蜂鸣器对应的 PWM 通道
    return 0;
}

static int buzzer_set(uint16_t freq, uint8_t volume)
{
    rt_uint32_t period, pulse;

    /* 将频率转化为周期 周期单位:ns 频率单位:HZ */
    period = 1000000000 / freq; // unit:ns 1/HZ*10^9 = ns

    /* 根据声音大小计算占空比 蜂鸣器低电平触发 */
    pulse = period - period / 100 * volume;

    /* 利用 PWM API 设定 周期和占空比 */
    rt_pwm_set(pwm_device,
               BUZZER_PWM_CH,
               period,
               pulse); // channel,period,pulse

    return 0;
}

static int buzzer_init(void)
{
    MX_TIM2_Init();
    return 0;
}

static uint16_t freq_tab[12]  = {262,
                                277,
                                294,
                                311,
                                330,
                                349,
                                369,
                                392,
                                415,
                                440,
                                466,
                                494}; //原始频率表 CDEFGAB
static uint8_t  buzzer_volume = 3;
static int      on_off;
#define AUTO_MODE 2

static void buzzer_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    on_off          = AUTO_MODE;
    /* 查找PWM设备 */
    pwm_device = (struct rt_device_pwm *)rt_device_find(BUZZER_PWM_DEVICE);
    if (pwm_device == RT_NULL) {
        rt_kprintf("pwm device %s not found!\n", BUZZER_PWM_DEVICE);
        return;
    }

    for (uint8_t i = 0; i < 12; i++) {
        buzzer_set(freq_tab[i], buzzer_volume);
        buzzer_on();

        rt_thread_mdelay(200);
        if (*ptr_work_status == 0)
            break;

        buzzer_off();
        rt_thread_mdelay(200);
        if (*ptr_work_status == 0)
            break;
    }
    on_off = 0;
}

static void buzzer_deinit(void)
{
    buzzer_off();
    rt_pin_mode(PA0, PIN_MODE_INPUT);
}

static int buzzer_write(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "on-off") == 0) {
            if (on_off == AUTO_MODE) {
                return -1;
            }
            if ((int)data == 1) {
                buzzer_set(freq_tab[9], buzzer_volume);
                buzzer_on();
                on_off = (int)data;
            } else if ((int)data == 0) {
                on_off = (int)data;
                buzzer_off();
            } else {
                return -1;
            }
            return 0;
        }
        return -1;
    }
    return 0;
}

static int buzzer_read(void *cmd, void *data)
{
    if ((uint32_t)cmd != 0) {
        if (rt_strcmp((char *)cmd, "on-off") == 0) {
            *(double *)data = on_off;
            return 0;
        }
        return -1;
    }
    return -1;
}

static const struct fro_module_ops buzzer_ops = {
    buzzer_init,
    buzzer_deinit,
    buzzer_run,
    buzzer_write,
    buzzer_read,
};

static int buzzer_module_init(void)
{
    buzzer.ops  = &buzzer_ops;
    buzzer.type = M_BUZZER;
    buzzer.name = "蜂鸣器模块";

    fro_module_register(&buzzer);
    return 0;
}
INIT_COMPONENT_EXPORT(buzzer_module_init);
