//-----------------------------------------------------------------------------
/*
   File        : fsmc.c
   Version     : V2.2
   By          : 银网科技

   Description :数据总线fsmc初始化
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "fsmc.h"

#include "gpio.h"

#include "DevRegs.h"
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
SRAM_HandleTypeDef hsram1;

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// FSMC initialization function
void MX_FSMC_Init(void)
{

  // Perform the SRAM1 memory initialization sequence
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK1;
  hsram1.Init.DataAddressMux      = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType          = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth     = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram1.Init.BurstAccessMode     = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity  = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode            = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive    = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation      = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal          = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode        = FSMC_EXTENDED_MODE_ENABLE;
  hsram1.Init.AsynchronousWait    = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst          = FSMC_WRITE_BURST_DISABLE;
  hsram1.Init.PageSize            = FSMC_PAGE_SIZE_NONE;

  FSMC_NORSRAM_TimingTypeDef Timing     = {0};
  FSMC_NORSRAM_TimingTypeDef ExtTiming  = {0};
  
  // Timing 用于读操作
  // LCD - ST7789S
  // MCU - 407    HCLK=168M  -> 5.95ns/period
  //   WRX: 
  //     Read cycle (ID)               >= 160ns
  //     Read cycle (FM)               >= 450ns
  Timing.AddressSetupTime         = 15;
  Timing.AddressHoldTime          = 15;
  Timing.DataSetupTime            = 60;
  Timing.BusTurnAroundDuration    = 0;
  Timing.CLKDivision              = 16;
  Timing.DataLatency              = 17;
  Timing.AccessMode               = FSMC_ACCESS_MODE_A;

  // ExtTiming 用于写操作
  // LCD - ST7789S
  // MCU - 407    HCLK=168M  -> 5.95ns/period
  //   WRX: 
  //     Control pulse “L/H” duration    15ns
  //        对应 DataSetupTime    选择5x5.95=23.8ns
  //     Write cycle              >= 66ns
  //     一次写入的时间：
  //   AddSet + DataSet + 1 = (4 + 7 + 1) * 5.95 = 12 * 5.95 = 71.4ns # 14.0M/s
  ExtTiming.AddressSetupTime      =  4;
  ExtTiming.AddressHoldTime       =  0;
  ExtTiming.DataSetupTime         =  7;
  ExtTiming.BusTurnAroundDuration =  0;
  ExtTiming.CLKDivision           =  0;
  ExtTiming.DataLatency           =  0;
  ExtTiming.AccessMode            = FSMC_ACCESS_MODE_A;

  if (HAL_SRAM_Init(&hsram1, &Timing, &ExtTiming) != HAL_OK)
    {
    SetHWFault( RHF_FSMC_ERR );
    }
}
//-----------------------------------------------------------------------------
static void HAL_FSMC_MspInit(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* Peripheral clock enable */
  __HAL_RCC_FSMC_CLK_ENABLE();
  
  /** FSMC GPIO Configuration
  PE7    ------> FSMC_D4
  PE8    ------> FSMC_D5
  PE9    ------> FSMC_D6
  PE10   ------> FSMC_D7
  PE11   ------> FSMC_D8
  PE12   ------> FSMC_D9
  PE13   ------> FSMC_D10
  PE14   ------> FSMC_D11
  PE15   ------> FSMC_D12
  PD8    ------> FSMC_D13
  PD9    ------> FSMC_D14
  PD10   ------> FSMC_D15
  PD11   ------> FSMC_A16  // LCD#CS
  PD12   ------> FSMC_A17  // LCDRS
  PD14   ------> FSMC_D0
  PD15   ------> FSMC_D1
  PD0    ------> FSMC_D2
  PD1    ------> FSMC_D3
  PD4    ------> FSMC_NOE
  PD5    ------> FSMC_NWE
  //PD7    ------> FSMC_NE1
  */
  /* GPIO_InitStruct */
  GPIO_InitStruct.Pin   = GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  |
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | 
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* GPIO_InitStruct */
  GPIO_InitStruct.Pin   = GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | 
#ifndef LCD_nCS_Pin

                          GPIO_PIN_11 | GPIO_PIN_12 | 
#endif
                          GPIO_PIN_14 | GPIO_PIN_15 | 
                          GPIO_PIN_0  | GPIO_PIN_1  | 
                          GPIO_PIN_4  | GPIO_PIN_5 | GPIO_PIN_7;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}
//-----------------------------------------------------------------------------
void HAL_SRAM_MspInit(SRAM_HandleTypeDef* sramHandle)
{

  HAL_FSMC_MspInit();
}
//-----------------------------------------------------------------------------
static void HAL_FSMC_MspDeInit(void)
{

}
//-----------------------------------------------------------------------------
void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef* sramHandle)
{

  HAL_FSMC_MspDeInit();
}
//-----------------------------------------------------------------------------
