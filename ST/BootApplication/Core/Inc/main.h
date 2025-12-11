/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f4xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define shield_SwS1_Pin GPIO_PIN_1
#define shield_SwS1_GPIO_Port GPIOA
#define shield_SwS1_EXTI_IRQn EXTI1_IRQn
#define shield_SwS2_Pin GPIO_PIN_4
#define shield_SwS2_GPIO_Port GPIOA
#define shield_SwS2_EXTI_IRQn EXTI4_IRQn
#define shield_LedD2_Pin GPIO_PIN_6
#define shield_LedD2_GPIO_Port GPIOA
#define shield_LedD3_Pin GPIO_PIN_7
#define shield_LedD3_GPIO_Port GPIOA
#define shield_SwS3_Pin GPIO_PIN_0
#define shield_SwS3_GPIO_Port GPIOB
#define shield_SwS3_EXTI_IRQn EXTI0_IRQn
#define shield_LedD4_Pin GPIO_PIN_6
#define shield_LedD4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
