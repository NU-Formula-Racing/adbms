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
#include "stm32h7xx_hal.h"

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
void MX_SPI1_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SD_CARD_DETECT_Pin GPIO_PIN_13
#define SD_CARD_DETECT_GPIO_Port GPIOC
#define SPI_CSB_Pin GPIO_PIN_4
#define SPI_CSB_GPIO_Port GPIOA
#define GPIO_LED_3_Pin GPIO_PIN_10
#define GPIO_LED_3_GPIO_Port GPIOE
#define GPIO_LED_2_Pin GPIO_PIN_11
#define GPIO_LED_2_GPIO_Port GPIOE
#define GPIO_LED_1_Pin GPIO_PIN_12
#define GPIO_LED_1_GPIO_Port GPIOE
#define Shutdown_Contactors_Pin GPIO_PIN_8
#define Shutdown_Contactors_GPIO_Port GPIOD
#define AD6822_State_Pin GPIO_PIN_9
#define AD6822_State_GPIO_Port GPIOD
#define CAN_RX_Pin GPIO_PIN_0
#define CAN_RX_GPIO_Port GPIOD
#define CAN_TX_Pin GPIO_PIN_1
#define CAN_TX_GPIO_Port GPIOD
#define IMD_STATUS_IN_Pin GPIO_PIN_6
#define IMD_STATUS_IN_GPIO_Port GPIOD
#define BMS_STATUS_OUT_Pin GPIO_PIN_7
#define BMS_STATUS_OUT_GPIO_Port GPIOD
#define TSSI_G_Pin GPIO_PIN_4
#define TSSI_G_GPIO_Port GPIOB
#define TSSI_R_Pin GPIO_PIN_5
#define TSSI_R_GPIO_Port GPIOB
#define CONTACTOR_N_CTRL_Pin GPIO_PIN_7
#define CONTACTOR_N_CTRL_GPIO_Port GPIOB
#define CONTACTOR_P_CTRL_Pin GPIO_PIN_8
#define CONTACTOR_P_CTRL_GPIO_Port GPIOB
#define CONTACTOR_PRE_CTRL_Pin GPIO_PIN_9
#define CONTACTOR_PRE_CTRL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
