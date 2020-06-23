/**
 * @file drv_servo.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-22
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "zl_servo.h"
#include <rtdevice.h>
#include <board.h>

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "DRV_SERVO"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

static TIM_HandleTypeDef htim1;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

struct _servo {
        struct zl_servo_device servo_device;
        struct zl_servo_parameter paras;
        char *name;
};

static struct _servo servo_obj[] = {
        { .paras = { .handler = (void *)&htim1,
                     .type = ZL_PWM_SERVO,
                     .channel = 1 },
          .name = "up_servo" },
        { .paras = { .handler = (void *)&htim1,
                     .type = ZL_PWM_SERVO,
                     .channel = 2 },
          .name = "down_servo" },
};

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
        /* USER CODE BEGIN TIM1_Init 0 */

        /* USER CODE END TIM1_Init 0 */

        TIM_MasterConfigTypeDef sMasterConfig = { 0 };
        TIM_OC_InitTypeDef sConfigOC = { 0 };
        TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

        /* USER CODE BEGIN TIM1_Init 1 */

        /* USER CODE END TIM1_Init 1 */
        htim1.Instance = TIM1;
        htim1.Init.Prescaler = 199;
        htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim1.Init.Period = 7199;
        htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim1.Init.RepetitionCounter = 0;
        htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
                Error_Handler();
        }
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) !=
            HAL_OK) {
                Error_Handler();
        }
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 0;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
        sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
        if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) !=
            HAL_OK) {
                Error_Handler();
        }
        if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) !=
            HAL_OK) {
                Error_Handler();
        }
        sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
        sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
        sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
        sBreakDeadTimeConfig.DeadTime = 0;
        sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
        sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
        sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
        if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) !=
            HAL_OK) {
                Error_Handler();
        }
        /* USER CODE BEGIN TIM1_Init 2 */

        /* USER CODE END TIM1_Init 2 */
        HAL_TIM_MspPostInit(&htim1);
}

static int _pwm_servo_set_pos(TIM_HandleTypeDef *htim, uint8_t channel,
                              uint16_t pos)
{
        __HAL_TIM_SET_COMPARE(htim, channel, 179 + (pos << 2));
        return 0;
}

static int _servo_set_pos(struct zl_servo_device *device, uint16_t pos)
{
        struct zl_servo_parameter *parameter =
                (struct zl_servo_parameter *)device->parent.user_data;
        if (parameter->type == ZL_PWM_SERVO) {
                TIM_HandleTypeDef *htim =
                        (TIM_HandleTypeDef *)parameter->handler;
                uint8_t ch = (parameter->channel - 1) << 2;
                _pwm_servo_set_pos(htim, ch, pos);
                device->config.pos = pos;
                return 0;
        }
        return -1;
}

static int _servo_read_pos(struct zl_servo_device *device, uint16_t *pos)
{
        struct zl_servo_parameter *parameter =
                (struct zl_servo_parameter *)device->parent.user_data;
        if (parameter->type == ZL_PWM_SERVO) {
                *pos = device->config.pos;
                return 0;
        }
        return -1;
}

static int _servo_enable(struct zl_servo_device *device, uint8_t enable)
{
        struct zl_servo_parameter *parameter =
                (struct zl_servo_parameter *)device->parent.user_data;
        if (parameter->type == ZL_PWM_SERVO) {
                TIM_HandleTypeDef *htim =
                        (TIM_HandleTypeDef *)parameter->handler;
                uint8_t ch = (parameter->channel - 1) << 2;
                if (enable) {
                        return HAL_TIM_PWM_Start(htim, ch);
                } else {
                        return HAL_TIM_PWM_Stop(htim, ch);
                }
        }
        return -1;
}

static struct zl_servo_ops drv_ops = { .set_pos = _servo_set_pos,
                                       .read_pos = _servo_read_pos,
                                       .enable = _servo_enable };

static int servo_init(void)
{
        int ret = RT_EOK;
        MX_TIM1_Init();

        for (int i = 0; i < sizeof(servo_obj) / sizeof(servo_obj[0]); i++) {
                if (zl_servo_register(&servo_obj[i].servo_device,
                                      servo_obj[i].name, &drv_ops,
                                      &servo_obj[i].paras) == RT_EOK) {
                        LOG_D("%s register success", servo_obj[i].name);
                } else {
                        LOG_E("%s register failed", servo_obj[i].name);
                        ret = -RT_ERROR;
                }
        }
        return ret;
}
INIT_DEVICE_EXPORT(servo_init);
