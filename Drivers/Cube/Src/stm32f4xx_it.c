//-----------------------------------------------------------------------------
/*
   File        : stm32f4xx_it.c
   Version     : V1.10
   By          : 银网科技

   Description :各种中断响应
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "stm32f4xx_it.h"

#include "DevRegs.h"
#include "DevClock.h"
#include "DevDebug.h"

#include "ADCMgr.h"
#include "NumProc.h"

#include "gpio.h"
#include "usart.h"
#include "UartChls.h"
#include "SPIChls.h"

#include "cmsis_os.h"
//=============================================================================
// 全局数据引用
//-----------------------------------------------------------------------------
extern SPI_HandleTypeDef hspi1;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

#if SYNC_LOGIC_EN == 0
  extern TIM_HandleTypeDef htim7;    // RT Calc
#endif

extern TIM_HandleTypeDef htim9;

extern DMA_HandleTypeDef hdma_adc1;  // DMA for ADC
//=============================================================================
// 系统中断
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{

  DEV_FAULT( GFC_HardFault );

  while (1)
    {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
    }
}
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{

  while (1)
    {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{

  while (1)
    {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
    }
}
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}
//-----------------------------------------------------------------------------
/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}
//=============================================================================
// 应用中断
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// This function handles TIM2 global interrupt.
// TIM2
void TIM2_IRQHandler(void)
{

  // TIM Update event
  if(__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET)
    {
    if(__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET)
      {
      __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
      
      // 溢出，无频率输入
      // 分频器按50Hz工作
      // 计算TIM3的输出频率
      uint32_t uTimFreq = 2 * HAL_RCC_GetPCLK1Freq(),   // TIM3在APB1上, 其频率的APB2的2倍
               uPeriod  = uTimFreq / 50;                // 默认频率 50Hz
      // 按ADC_SAMPLES_PER_PEROID分频
      TIM3->ARR = (uPeriod + (NUM_SAMPLES_PER_PEROID / 2)) / NUM_SAMPLES_PER_PEROID;  

      // uPeriod /= (uTimFreq / 1000000);  // 以us为单位
//      DevCache.Common[REG_ACPeriod - REG_COMMON] = 0;
//      DevCache.Common[REG_VOPeriod - REG_COMMON] = 0;
      DevCache.Reals[REG_RL_ACPeriod - REG_REAL] = 0;
      DevCache.Reals[REG_RL_VOPeriod - REG_REAL] = 0;

      // 标记
      SetHWFault( RHF_Freq_MIS );
      }
    }
  else   
    HAL_TIM_IRQHandler(&htim2);
}
//-----------------------------------------------------------------------------
// This function handles TIM3 global interrupt.
// 分频启动采样
void TIM3_IRQHandler(void)
{

  HAL_TIM_IRQHandler(&htim3);
  
  // 启动采样
  ADC_Startup();

  //LED_STB_ON;
}
//-----------------------------------------------------------------------------
#if SYNC_LOGIC_EN == 0
// TIM7
// AD采集完成后, 由TIM7驱动实时计算和保护逻辑
void TIM7_IRQHandler(void)
{
  
  HAL_TIM_IRQHandler(&htim7);

  /* USER CODE BEGIN TIM7_IRQn 1 */ 
#if DBG_TIME_MEASURE > 0
  // debug only
  uint32_t FuTIMStart = htim2.Instance->CNT;  
#endif

  // 保护数据处理
  NumProc_ProtectCalcIT();

#if DBG_TIME_MEASURE > 0
  // debug only
  uint32_t FuTIMEnd = htim2.Instance->CNT,
           uTimFreq = 2 * HAL_RCC_GetPCLK1Freq() / 1000000;
  if( FuTIMEnd > FuTIMStart )
    _SetCommonReg( REG_DEV_Probe1, (FuTIMEnd - FuTIMStart) / uTimFreq );
  else
    _SetCommonReg( REG_DEV_Probe1, ((0xFFFFFFFF - FuTIMStart) + FuTIMEnd) / uTimFreq );
#endif
}
#endif // SYNC_LOGIC_EN
//-----------------------------------------------------------------------------
// This function handles TIM1 break interrupt and TIM9 global interrupt.
// RTOS用的1ms定时器
// 由RTOS的port模块完成初始化
void TIM1_BRK_TIM9_IRQHandler(void)
{

  HAL_TIM_IRQHandler(&htim9);
  
  // 
  DEVCLK_Tick( &DevClock );
  
//  LED_STB_TOGGLE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// SPI1
//-----------------------------------------------------------------------------
// This function handles SPI1 global interrupt.
// SPI1: 用于W5500以太网接口
void SPI1_IRQHandler(void)
{

  HAL_SPI_IRQHandler( &(SPICtrls[SPI1_CHL].SPI) );
}
//-----------------------------------------------------------------------------
// This function handles DMA2 stream0 global interrupt.
// DMA2_0 SPI1.rx
void DMA2_Stream0_IRQHandler(void)
{

  HAL_DMA_IRQHandler( &SPICtrls[SPI1_CHL].rxDMA );
}
//-----------------------------------------------------------------------------
// This function handles DMA2 stream3 global interrupt.
// DMA2_3 SPI1.tx
void DMA2_Stream3_IRQHandler(void)
{

  HAL_DMA_IRQHandler( &SPICtrls[SPI1_CHL].txDMA );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{

  UART_IRQHandler( UART1_CHL );
}
//-----------------------------------------------------------------------------
/**
* @brief This function handles DMA2 stream2 global interrupt.
  UART1.Rx
*/
void DMA2_Stream2_IRQHandler(void)
{

  HAL_DMA_IRQHandler( &GetUARTChl(UART1_CHL).rxDMA );
}
//-----------------------------------------------------------------------------
/**
* @brief This function handles DMA2 stream7 global interrupt.
  UART1.Tx
*/
void DMA2_Stream7_IRQHandler(void)
{
  
  TUARTCtrlBlock *pcbUART = &GetUARTChl(UART1_CHL);

  if( DMA_IT_TC == __HAL_DMA_GET_IT_SOURCE(&(pcbUART->txDMA), DMA_IT_TC) )
    {
    pcbUART->usState |= USB_CPLSENT;
    }

  HAL_DMA_IRQHandler( &(pcbUART->txDMA) );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// This function handles DMA2 stream4 global interrupt.
// DMA2_4 ADC1
void DMA2_Stream4_IRQHandler(void)
{

  HAL_DMA_IRQHandler(&hdma_adc1);

  //LED_STB_OFF;
}
//-----------------------------------------------------------------------------
