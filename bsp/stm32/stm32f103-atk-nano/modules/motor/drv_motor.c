/**
 * @file drv_motor.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-23
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "zl_motor.h"
#include <rtdevice.h>
#include <board.h>

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "DRV_MOTOR"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

static TIM_HandleTypeDef htim8;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

struct _drv8872_motor {
        struct zl_motor_device motor_device;
        struct zl_motor_parameter paras;
        char *name;
};

static struct _drv8872_motor drv8872_motor_obj[] = {
        { .paras = { .type = ZL_DRV8872_MOTOR,
                     .inverse = 0,
                     .ch1 = 3,
                     .ch2 = 4,
                     .handler = (void *)&htim8, },
          .name = "right", },
        { .paras = { .type = ZL_DRV8872_MOTOR,
                     .inverse = 1,
                     .ch1 = 1,
                     .ch2 = 2,
                     .handler = (void *)&htim8, },
          .name = "left", }
};

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{
        /* USER CODE BEGIN TIM8_Init 0 */

        /* USER CODE END TIM8_Init 0 */

        TIM_MasterConfigTypeDef sMasterConfig = { 0 };
        TIM_OC_InitTypeDef sConfigOC = { 0 };
        TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

        /* USER CODE BEGIN TIM8_Init 1 */

        /* USER CODE END TIM8_Init 1 */
        htim8.Instance = TIM8;
        htim8.Init.Prescaler = 199;
        htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim8.Init.Period = 4095;
        htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim8.Init.RepetitionCounter = 0;
        htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_PWM_Init(&htim8) != HAL_OK) {
                Error_Handler();
        }
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) !=
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
        if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) !=
            HAL_OK) {
                Error_Handler();
        }
        if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) !=
            HAL_OK) {
                Error_Handler();
        }
        if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_3) !=
            HAL_OK) {
                Error_Handler();
        }
        if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_4) !=
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
        if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) !=
            HAL_OK) {
                Error_Handler();
        }
        /* USER CODE BEGIN TIM8_Init 2 */

        /* USER CODE END TIM8_Init 2 */
        HAL_TIM_MspPostInit(&htim8);
}

/**
 * @brief 
 * 
 * in1 in2 out1 out2
 * 0    0   Z   Z   Coast
 * 0    1   L   H   Reverse
 * 1    0   H   H   Forward
 * 1    1   L   L   Brake
 * 
 * @param htim 
 * @param in1 
 * @param in2 
 * @param speed 
 * @return int 
 */
static int _drv8872_motor_set_throttle(TIM_HandleTypeDef *htim, uint8_t in1,
                                       uint8_t in2, int16_t speed)
{
        if (speed == ZL_MOTOR_BRAKE) {
                // all pins output high
                __HAL_TIM_SET_COMPARE(htim, in1, 4096);
                __HAL_TIM_SET_COMPARE(htim, in2, 4096);
        }
        if (speed >= 0) {
                __HAL_TIM_SET_COMPARE(htim, in2, 0); // output low
                __HAL_TIM_SET_COMPARE(htim, in1, speed);
        }
        if (speed < 0) {
                __HAL_TIM_SET_COMPARE(htim, in1, 0); // output low
                __HAL_TIM_SET_COMPARE(htim, in2, -speed);
        }
        return 0;
}

static int _motor_set_throttle(struct zl_motor_device *device, int16_t speed)
{
        struct zl_motor_parameter *parameter =
                (struct zl_motor_parameter *)device->parent.user_data;
        if (parameter->type == ZL_DRV8872_MOTOR) {
                TIM_HandleTypeDef *htim =
                        (TIM_HandleTypeDef *)parameter->handler;
                uint8_t ch1 = (parameter->ch1 - 1) << 2;
                uint8_t ch2 = (parameter->ch2 - 1) << 2;
                device->config.speed = speed;
                if (parameter->inverse) {
                        return _drv8872_motor_set_throttle(htim, ch1, ch2,
                                                           -speed);
                } else {
                        return _drv8872_motor_set_throttle(htim, ch1, ch2,
                                                           speed);
                }
        }
        return -1;
}

static int _motor_read_throttle(struct zl_motor_device *device, int16_t *speed)
{
        struct zl_motor_parameter *parameter =
                (struct zl_motor_parameter *)device->parent.user_data;
        if (parameter->type == ZL_DRV8872_MOTOR) {
                *speed = device->config.speed;
                return 0;
        }
        return -1;
}

static int _motor_enable(struct zl_motor_device *device, uint8_t enable)
{
        struct zl_motor_parameter *parameter =
                (struct zl_motor_parameter *)device->parent.user_data;
        if (parameter->type == ZL_DRV8872_MOTOR) {
                TIM_HandleTypeDef *htim =
                        (TIM_HandleTypeDef *)parameter->handler;
                uint8_t ch1 = (parameter->ch1 - 1) << 2;
                uint8_t ch2 = (parameter->ch2 - 1) << 2;
                if (enable) {
                        HAL_TIM_PWM_Start(htim, ch1);
                        return HAL_TIM_PWM_Start(htim, ch2);
                } else {
                        HAL_TIM_PWM_Stop(htim, ch1);
                        return HAL_TIM_PWM_Stop(htim, ch2);
                }
        }
        return -1;
}

static struct zl_motor_ops drv_ops = {
        .set_throttle = _motor_set_throttle,
        .read_throttle = _motor_read_throttle,
        .enable = _motor_enable,
};

static int motor_init(void)
{
        int ret = RT_EOK;

        MX_TIM8_Init();

        for (int i = 0;
             i < sizeof(drv8872_motor_obj) / sizeof(drv8872_motor_obj[0]);
             i++) {
                if (zl_motor_register(&drv8872_motor_obj[i].motor_device,
                                      drv8872_motor_obj[i].name, &drv_ops,
                                      &drv8872_motor_obj[i].paras) == RT_EOK) {
                        LOG_D("%s register success", drv8872_motor_obj[i].name);
                } else {
                        LOG_E("%s register failed", drv8872_motor_obj[i].name);
                        ret = -RT_ERROR;
                }
        }
        return ret;
}
INIT_DEVICE_EXPORT(motor_init);
