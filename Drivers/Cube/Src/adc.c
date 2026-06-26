//-----------------------------------------------------------------------------
/*
   File       : adc.c
   Version    : V2.2
   By         : 银网科技

   Description :ADC初始化
          
   Date       : 2018.12.11
*/
//-----------------------------------------------------------------------------
#include "adc.h"

#include "gpio.h"

#include "ADCMgr.h"
#include "DevRegs.h"
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;

DMA_HandleTypeDef hdma_adc1;
//DMA_HandleTypeDef hdma_adc2;
//DMA_HandleTypeDef hdma_adc3;
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// ADC1 init function
void MX_ADC1_Init(void)
{

  // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; //  APB2(84M) / 6 = 14M
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode          = ENABLE;
  hadc1.Init.ContinuousConvMode    = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
//  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
//  hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_CC1;
  hadc1.Init.ExternalTrigConvEdge   = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv       = ADC_SOFTWARE_START;   // 软件启动

  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion       = ADC_NbrOfConversion;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }

  // Configure the ADC multi-mode 
  // TWOSAMPLINGDELAY = (SamplingTime + 12) / 3
  ADC_MultiModeTypeDef multimode = {0};
  multimode.Mode             = ADC_TRIPLEMODE_REGSIMULT; // ADC_DUALMODE_REGSIMULT;
  multimode.DMAAccessMode    = ADC_DMAACCESSMODE_1;      // ADC_DMAACCESSMODE_2; DMA模式1用于三重规则同时模式
  multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_9CYCLES; // ADC_TWOSAMPLINGDELAY_5CYCLES;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }
}
//-----------------------------------------------------------------------------
// ADC2 init function
void MX_ADC2_Init(void)
{
  
  // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler         = ADC_CLOCK_SYNC_PCLK_DIV4; //  APB2(84M) / 6 = 14M
  hadc2.Init.Resolution             = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode           = ENABLE;
  hadc2.Init.ContinuousConvMode     = DISABLE;
  hadc2.Init.DiscontinuousConvMode  = DISABLE;
  hadc2.Init.DataAlign              = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion        = ADC_NbrOfConversion;
//  hadc2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
//  hadc2.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_CC1;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }

  ADC_MultiModeTypeDef multimode = {0};
  multimode.Mode = ADC_TRIPLEMODE_REGSIMULT;         // ADC_DUALMODE_REGSIMULT;
  multimode.DMAAccessMode    = ADC_DMAACCESSMODE_1;  // ADC_DMAACCESSMODE_2;  DMA模式1用于三重规则同时模式
  multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_9CYCLES; //ADC_TWOSAMPLINGDELAY_5CYCLES;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc2, &multimode) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }
}
//-----------------------------------------------------------------------------
// ADC3 init function
void MX_ADC3_Init(void)
{

  // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; //  APB2(84M) / 6 = 14M
  hadc3.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode          = ENABLE;
  hadc3.Init.ContinuousConvMode    = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion       = ADC_NbrOfConversion;
//  hadc3.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
//  hadc3.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_CC1;
  hadc3.Init.DMAContinuousRequests = ENABLE;
  hadc3.Init.EOCSelection          = ADC_EOC_SEQ_CONV;;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }

  ADC_MultiModeTypeDef multimode = {0};
  multimode.Mode             = ADC_TRIPLEMODE_REGSIMULT; // ADC_DUALMODE_REGSIMULT;
  multimode.DMAAccessMode    = ADC_DMAACCESSMODE_1;      // ADC_DMAACCESSMODE_2; DMA模式1用于三重规则同时模式
  multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_9CYCLES; //ADC_TWOSAMPLINGDELAY_5CYCLES;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc3, &multimode) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }
}
//-----------------------------------------------------------------------------
// HAL的初始化回调
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
  
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**ADC1 GPIO Configuration    
    PA0     ------> ADC123_IN0    Tin0
    PC2     ------> ADC123_IN12   Tin1
    PC0     ------> ADC123_IN10   Vin0
    PC1     ------> ADC123_IN11   Vin1
    */
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = AVin0_Pin;
    HAL_GPIO_Init(AVin0_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = AVin1_Pin;
    HAL_GPIO_Init(AVin1_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = ATin0_Pin;
    HAL_GPIO_Init(ATin0_Port, &GPIO_InitStruct);
#ifdef ATin1_Pin
    GPIO_InitStruct.Pin = ATin1_Pin;
    HAL_GPIO_Init(ATin1_Port, &GPIO_InitStruct);
#endif

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream4;
    hdma_adc1.Init.Channel    = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction  = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc  = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc     = DMA_MINC_ENABLE;
    hdma_adc1.Init.Mode       = DMA_CIRCULAR; //DMA_NORMAL; //DMA_CIRCULAR;
    hdma_adc1.Init.Priority   = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode   = DMA_FIFOMODE_ENABLE;  //DMA_FIFOMODE_DISABLE;
    hdma_adc1.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; //DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_PDATAALIGN_HALFWORD; //DMA_MDATAALIGN_WORD;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
      {
      SetHWFault( RHF_ADC_ERR );
      }

    __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc1);
    }
  else if(adcHandle->Instance==ADC2)
    {
    /* ADC2 clock enable */
    __HAL_RCC_ADC2_CLK_ENABLE();
    }
  else if(adcHandle->Instance==ADC3)
    {
    __HAL_RCC_ADC3_CLK_ENABLE();
    }
}
//-----------------------------------------------------------------------------
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
} 
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// ADC初始化
void MX_ADC_Init(void)
{
  
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_ADC3_Init();
  
  ADC_SetSamplesBanks( 0 );
}
//-----------------------------------------------------------------------------
// 设置采样序列
void ADC_SetSamplesBanks(uint32_t uOrder)
{

  ADC_ChannelConfTypeDef sConfig = {0};

  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  for( uint32_t uIdx = 0; uIdx < ADC_NbrOfConversion; uIdx++ )
    {
    sConfig.Rank = uIdx + 1;
    sConfig.Channel = FucADCChannels[uIdx][0];
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
      {
      SetHWFault( RHF_ADC_ERR );
      }

    sConfig.Channel = FucADCChannels[uIdx][1];
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
      {
      SetHWFault( RHF_ADC_ERR );
      }

    sConfig.Channel = FucADCChannels[uIdx][2];
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
      {
      SetHWFault( RHF_ADC_ERR );
      }
    }
}
//-----------------------------------------------------------------------------
