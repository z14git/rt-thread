/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define nFault_3_Pin GPIO_PIN_13
#define nFault_3_GPIO_Port GPIOC
#define PUMP_IN_Pin GPIO_PIN_2
#define PUMP_IN_GPIO_Port GPIOC
#define PUMP_OUT_Pin GPIO_PIN_3
#define PUMP_OUT_GPIO_Port GPIOC
#define PD_Pin GPIO_PIN_4
#define PD_GPIO_Port GPIOA
#define nFault_4_Pin GPIO_PIN_5
#define nFault_4_GPIO_Port GPIOA
#define servo4_Pin GPIO_PIN_6
#define servo4_GPIO_Port GPIOA
#define servo3_Pin GPIO_PIN_7
#define servo3_GPIO_Port GPIOA
#define servo2_Pin GPIO_PIN_0
#define servo2_GPIO_Port GPIOB
#define servo1_Pin GPIO_PIN_1
#define servo1_GPIO_Port GPIOB
#define state_led_Pin GPIO_PIN_2
#define state_led_GPIO_Port GPIOB
#define RGB2_Pin GPIO_PIN_10
#define RGB2_GPIO_Port GPIOB
#define RGB1_Pin GPIO_PIN_11
#define RGB1_GPIO_Port GPIOB
#define M2_IN1_Pin GPIO_PIN_6
#define M2_IN1_GPIO_Port GPIOC
#define M2_IN2_Pin GPIO_PIN_7
#define M2_IN2_GPIO_Port GPIOC
#define M1_IN1_Pin GPIO_PIN_8
#define M1_IN1_GPIO_Port GPIOC
#define M1_IN2_Pin GPIO_PIN_9
#define M1_IN2_GPIO_Port GPIOC
#define UP_SERVO_Pin GPIO_PIN_8
#define UP_SERVO_GPIO_Port GPIOA
#define DOWN_SERVO_Pin GPIO_PIN_9
#define DOWN_SERVO_GPIO_Port GPIOA
#define nFault_2_Pin GPIO_PIN_10
#define nFault_2_GPIO_Port GPIOA
#define nFault_1_Pin GPIO_PIN_11
#define nFault_1_GPIO_Port GPIOA
#define serial_servo_Pin GPIO_PIN_10
#define serial_servo_GPIO_Port GPIOC
#define CLK_Pin GPIO_PIN_3
#define CLK_GPIO_Port GPIOB
#define CS_Pin GPIO_PIN_4
#define CS_GPIO_Port GPIOB
#define DIN_Pin GPIO_PIN_5
#define DIN_GPIO_Port GPIOB
#define DEBUG_TX_Pin GPIO_PIN_6
#define DEBUG_TX_GPIO_Port GPIOB
#define DEBUG_RX_Pin GPIO_PIN_7
#define DEBUG_RX_GPIO_Port GPIOB
#define M3_IN1_Pin GPIO_PIN_8
#define M3_IN1_GPIO_Port GPIOB
#define M3_IN2_Pin GPIO_PIN_9
#define M3_IN2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
