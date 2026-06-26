/**
  ******************************************************************************
  * File Name          : IWDG.c
  * Description        : This file provides code for the configuration
  *                      of the IWDG instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "iwdg.h"

IWDG_HandleTypeDef hiwdg;

/* IWDG init function */
void MX_IWDG_Init(void)
{

  /*##-3- Configure the IWDG peripheral ######################################*/
  /* Set counter reload value to obtain 500ms IWDG TimeOut.
     IWDG counter clock Frequency = LsiFreq / 32
     Counter Reload Value = 4000ms / IWDG counter clock period
                          = 4s / (32/LsiFreq)
                          = 4 * LsiFreq / 32
                          = LsiFreq / 8 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32; // LSI=32K， IWDG周期1ms 32分频=1ms
  hiwdg.Init.Reload    = 3000;              // 2000ms // 0~0xFFF
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
    }
}

void IWDG_FeedDog()
{

  HAL_IWDG_Refresh( &hiwdg );
}  
