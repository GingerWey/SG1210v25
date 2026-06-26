//-----------------------------------------------------------------------------
/*
   File        : TIM.c
   Version     : V2.2
   By          : 银网科技

   Description :定时器TIM初始化
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "tim.h"

#include "gpio.h"
#include "DevRegs.h"
#include "Dev_cfg.h"
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
TIM_HandleTypeDef htim2;    // 频率测量  TIM2_CH3、CH4
TIM_HandleTypeDef htim3;    // 采样分频  TIM1_CH1

#if SYNC_LOGIC_EN == 0
  TIM_HandleTypeDef htim7;      // RT Calc
#endif
//=============================================================================
// 局部方法申明
//-----------------------------------------------------------------------------
void MX_TIM2_Init(void);      // 采样分频
void MX_TIM3_Init(void);      

#if SYNC_LOGIC_EN == 0
  void MX_TIM7_Init(void);;      // RT Calc
#endif
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化各定时器
void MX_TIMs_Init(void)
{
  
  MX_TIM2_Init();
  MX_TIM3_Init();

#if SYNC_LOGIC_EN == 0
  MX_TIM7_Init();      // RT Calc
#endif
}
//=============================================================================
// 局部方法
//-----------------------------------------------------------------------------
// TIM2 init function
// 用于频率捕获
// APB1 Timer = 84MHz
void MX_TIM2_Init(void)
{

 /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler   = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period      = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }
    
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }
    
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}
//-----------------------------------------------------------------------------
/* TIM3 init function */
// TIM3  16位Timer
// 用于分频驱动采样
// APB1 Timer = 84MHz
void MX_TIM3_Init(void)
{

  // TIM3在APB1上, 其频率的APB1的2倍
  uint32_t uTimFreq = 2 * HAL_RCC_GetPCLK1Freq(), 
           uPeriod  = uTimFreq / 50;              // 默认电网频率 50Hz
  // 按NUM_SAMPLES_PER_PEROID分频
  uPeriod = (uPeriod + (NUM_SAMPLES_PER_PEROID - 1)) / NUM_SAMPLES_PER_PEROID;  

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = uPeriod - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
    SetHWFault( RHF_TIM_ERR );
    }
}
//-----------------------------------------------------------------------------
#if SYNC_LOGIC_EN == 0
// TIM7 init function
// 驱动实时计算
// APB1 Timer 84MHz
void MX_TIM7_Init(void)
{

  htim7.Instance = TIM7;
  htim7.Init.Prescaler   = 84;         // APB1 Timer Clock = 84M
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period      = 10;         // 10us
  htim7.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
    {
#ifdef __DEBUG      
    SetHWFault( RHF_TIM_ERR );
#endif      
    }

  if (HAL_TIM_OnePulse_Init(&htim7, TIM_OPMODE_SINGLE) != HAL_OK)
    {
#ifdef __DEBUG      
    SetHWFault( RHF_TIM_ERR );
#endif      
    }
}
#endif
//=============================================================================
// HAL回调
//-----------------------------------------------------------------------------
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspInit 0 */

  /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PB10     ------> TIM2_CH3
    PB11     ------> TIM2_CH4
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    GPIO_InitStruct.Pin = Freq0_Pin | Freq1_Pin;
    HAL_GPIO_Init(Freq0_Port, &GPIO_InitStruct);

    /* TIM2 interrupt Init */
    HAL_NVIC_SetPriority(TIM2_IRQn, 5, 1);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  /* USER CODE BEGIN TIM2_MspInit 1 */

  /* USER CODE END TIM2_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* TIM3 interrupt Init */
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
#if SYNC_LOGIC_EN == 0
  else if(tim_baseHandle->Instance == TIM7)
    {
  /* USER CODE BEGIN TIM7_MspInit 0 */

  /* USER CODE END TIM7_MspInit 0 */
    /* TIM7 clock enable */
    __HAL_RCC_TIM7_CLK_ENABLE();

    /* TIM7 interrupt Init */
    HAL_NVIC_SetPriority(TIM7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
  /* USER CODE BEGIN TIM7_MspInit 1 */

  /* USER CODE END TIM7_MspInit 1 */
    }
#endif
}
//-----------------------------------------------------------------------------
