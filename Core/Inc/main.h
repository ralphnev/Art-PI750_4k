/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BlankTime (1800-1)
#define Usec 100
#define TIMEDenom 8
#define Exposure (8000)
#define NumLines 1000
#define TIMENumer 5
#define Npixels 4096
#define TS_RST_Pin GPIO_PIN_3
#define TS_RST_GPIO_Port GPIOD
#define TS_IIC_INT_Pin GPIO_PIN_12
#define TS_IIC_INT_GPIO_Port GPIOG
#define Enable9_Pin GPIO_PIN_14
#define Enable9_GPIO_Port GPIOG
#define ROG_Pin GPIO_PIN_10
#define ROG_GPIO_Port GPIOA
#define BLUEled_Pin GPIO_PIN_8
#define BLUEled_GPIO_Port GPIOI
#define LED8_Pin GPIO_PIN_12
#define LED8_GPIO_Port GPIOA
#define LED7_Pin GPIO_PIN_11
#define LED7_GPIO_Port GPIOA
#define sensorClk_Pin GPIO_PIN_6
#define sensorClk_GPIO_Port GPIOC
#define DMAxfr_Pin GPIO_PIN_7
#define DMAxfr_GPIO_Port GPIOG
#define SD_CD_Pin GPIO_PIN_3
#define SD_CD_GPIO_Port GPIOG
#define LED3_Pin GPIO_PIN_1
#define LED3_GPIO_Port GPIOC
#define JOYstick2_Pin GPIO_PIN_2
#define JOYstick2_GPIO_Port GPIOH
#define LED5_Pin GPIO_PIN_2
#define LED5_GPIO_Port GPIOA
#define JOYstick1_Pin GPIO_PIN_3
#define JOYstick1_GPIO_Port GPIOH
#define TimeSet_Pin GPIO_PIN_4
#define TimeSet_GPIO_Port GPIOH
#define ADCCplt_Pin GPIO_PIN_10
#define ADCCplt_GPIO_Port GPIOH
#define LCD_SCL_Pin GPIO_PIN_11
#define LCD_SCL_GPIO_Port GPIOH
#define JOYstickPB_Pin GPIO_PIN_6
#define JOYstickPB_GPIO_Port GPIOA
#define LED4_Pin GPIO_PIN_7
#define LED4_GPIO_Port GPIOA
#define JOYstick3_Pin GPIO_PIN_2
#define JOYstick3_GPIO_Port GPIOB
#define Trigger_Pin GPIO_PIN_9
#define Trigger_GPIO_Port GPIOH
#define LCD_SDA_Pin GPIO_PIN_12
#define LCD_SDA_GPIO_Port GPIOH
#define SDwrt_Pin GPIO_PIN_13
#define SDwrt_GPIO_Port GPIOD
#define LED1_Pin GPIO_PIN_4
#define LED1_GPIO_Port GPIOC
#define JOYstick4_Pin GPIO_PIN_1
#define JOYstick4_GPIO_Port GPIOB
#define Tim8Cplt_Pin_Pin GPIO_PIN_8
#define Tim8Cplt_Pin_GPIO_Port GPIOH
#define LED6_Pin GPIO_PIN_3
#define LED6_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_5
#define LED2_GPIO_Port GPIOC
#define SensorPWRenable_Pin GPIO_PIN_0
#define SensorPWRenable_GPIO_Port GPIOB
#define Tim1Cplt_Pin GPIO_PIN_7
#define Tim1Cplt_GPIO_Port GPIOH

/* USER CODE BEGIN Private defines */
//#ifdef Npixels
//#undef Npixels
//#define Npixels (8192)
//#endif
#ifdef Exposure
#undef Exposure
#undef NumLines
extern uint32_t Exposure ;
extern uint32_t NumLines ;

#endif

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
