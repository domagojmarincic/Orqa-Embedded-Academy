/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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
#define LED_GREEN_Pin GPIO_PIN_13
#define LED_GREEN_GPIO_Port GPIOC
#define SW_2_Pin GPIO_PIN_1
#define SW_2_GPIO_Port GPIOA
#define test_led_Pin GPIO_PIN_5
#define test_led_GPIO_Port GPIOA
#define LED_2_Pin GPIO_PIN_10
#define LED_2_GPIO_Port GPIOB
#define VBUS_EN_Pin GPIO_PIN_8
#define VBUS_EN_GPIO_Port GPIOA
#define VBUS_SENSE_Pin GPIO_PIN_9
#define VBUS_SENSE_GPIO_Port GPIOA
#define SPI2_CS_Pin GPIO_PIN_0
#define SPI2_CS_GPIO_Port GPIOD
#define LED_3_Pin GPIO_PIN_3
#define LED_3_GPIO_Port GPIOB
#define LED_1_Pin GPIO_PIN_4
#define LED_1_GPIO_Port GPIOB
#define SW_1_Pin GPIO_PIN_5
#define SW_1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
