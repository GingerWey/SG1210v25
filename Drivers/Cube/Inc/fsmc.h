//-----------------------------------------------------------------------------
/*
   File        : fsmc.h
   Version     : V2.2
   By          : 银网科技

   Description :数据总线fsmc初始化
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef __FSMC_H
#define __FSMC_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern SRAM_HandleTypeDef hsram1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_FSMC_Init(void);
   
void HAL_SRAM_MspInit(SRAM_HandleTypeDef* hsram);
void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef* hsram);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__FSMC_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
