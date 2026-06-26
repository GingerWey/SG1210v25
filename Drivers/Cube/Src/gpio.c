//-----------------------------------------------------------------------------
/*
   File        : gpio.c
   Version     : V2.2
   By          : 银网科技

   Description :初始化功能管脚

   Date       : 2023.12.05
   
   2024/10/02
     Ver： v3.0
     适配HW2.0
*/
//-----------------------------------------------------------------------------
#include "gpio.h"
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Ports Clock Enable
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  //------------ 控制信号 -----------
  // MCU Power Keep ON
  KeepMCUPower_ON;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = KeepPowON_Pin;
  HAL_GPIO_Init(KeepPowON_Port, &GPIO_InitStruct);

  // AC Output mode control
  ACout_Passby_ON;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = ACout_nPassby_Pin;
  HAL_GPIO_Init(ACout_nPassby_Port, &GPIO_InitStruct);

  // Invertor enable control
  Invertor_OFF;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = Invertor_En_Pin;
  HAL_GPIO_Init(Invertor_En_Port, &GPIO_InitStruct);

  // Device Fan control
  DevFanCtrl_OFF;
  GPIO_InitStruct.Pin   = DevFanCtrl_Pin;
  HAL_GPIO_Init(DevFanCtrl_Port, &GPIO_InitStruct);

  // Relay control
  Trip1_OFF;
  GPIO_InitStruct.Pin   = Trip1_Pin;
  HAL_GPIO_Init(Trip1_Port, &GPIO_InitStruct);

  // Heater control
  Heater_OFF;
  GPIO_InitStruct.Pin   = Heater_Pin;
  HAL_GPIO_Init(Heater_Port, &GPIO_InitStruct);

  //------------ 状态信号 -----------
  // AC adapter avalidable 
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Pin   = Adapter_ON_Pin;
  HAL_GPIO_Init(Adapter_ON_Port, &GPIO_InitStruct);

  // Invertor 5V power supply avalidable 
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Pin   = Invertor5V_Pin;
  HAL_GPIO_Init(Invertor5V_Port, &GPIO_InitStruct);

  //------------ RTC -----------
  // RTC
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  RTC_SCL_HIGH;
  GPIO_InitStruct.Pin   = RTC_SCL_Pin;
  HAL_GPIO_Init(RTC_SCL_Port, &GPIO_InitStruct);
  RTC_SDA_HIGH;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pin   = RTC_SDA_Pin;
  HAL_GPIO_Init(RTC_SDA_Port, &GPIO_InitStruct);

  //------------ 功率板I2C -----------
  // Slave board IIC
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  SI2C_SCL_HIGH;
  GPIO_InitStruct.Pin   = SI2C_SCL_Pin;
  HAL_GPIO_Init(SI2C_SCL_Port, &GPIO_InitStruct);
  SI2C_SDA_HIGH;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pin   = SI2C_SDA_Pin;
  HAL_GPIO_Init(SI2C_SDA_Port, &GPIO_InitStruct);

  //------------ NVRAM -----------
  // FeRAM
  FeRAM_nCS_HIGH;
  FeRAM_nWP_LOW;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Pin   = FeRAM_nCS_Pin;
  HAL_GPIO_Init(FeRAM_nCS_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = FeRAM_nWP_Pin;
  HAL_GPIO_Init(FeRAM_nWP_Port, &GPIO_InitStruct);

  // Flash
  FLASH_nCS_HIGH;
  FLASH_nWP_LOW;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Pin   = FLASH_nCS_Pin;
  HAL_GPIO_Init(FLASH_nCS_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Pin   = FLASH_nWP_Pin;
  HAL_GPIO_Init(FLASH_nWP_Port, &GPIO_InitStruct);

  //------------ LCD -----------
  // LCD
  LCD_Lamp_OFF;
  LCD_nRST_HIGH;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = LCD_nLamp_Pin;
  HAL_GPIO_Init(LCD_nLamp_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = LCD_nRST_Pin;
  HAL_GPIO_Init(LCD_nRST_Port, &GPIO_InitStruct);
#ifdef LCD_nCS_Pin
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pin   = LCD_nCS_Pin;
  HAL_GPIO_Init(LCD_nCS_Port, &GPIO_InitStruct);
#endif
#ifdef LCD_RS_Pin
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pin   = LCD_RS_Pin;
  HAL_GPIO_Init(LCD_RS_Port, &GPIO_InitStruct);
#endif

  //------------ USART1 -----------
  // USART
  USART1_TxEn_OFF;   // 低电平为接收使能
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = USART1_TxEn_Pin;
  HAL_GPIO_Init(USART1_TxEn_Port, &GPIO_InitStruct);

  //------------ LEDs -----------
  // Status LEDs
  LED1_OFF;
  LED2_OFF;
  LED3_OFF;
  LED4_OFF;
  LED5_OFF;
  LED6_OFF;
  LED7_OFF;
  LED8_OFF;
  LED9_OFF;
  LED10_OFF;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = M_nLED1_Pin;
  HAL_GPIO_Init(M_nLED1_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED2_Pin;
  HAL_GPIO_Init(M_nLED2_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED3_Pin;
  HAL_GPIO_Init(M_nLED3_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED4_Pin;
  HAL_GPIO_Init(M_nLED4_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED5_Pin;
  HAL_GPIO_Init(M_nLED5_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED6_Pin;
  HAL_GPIO_Init(M_nLED6_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED7_Pin;
  HAL_GPIO_Init(M_nLED7_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED8_Pin;
  HAL_GPIO_Init(M_nLED8_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED9_Pin;
  HAL_GPIO_Init(M_nLED9_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_nLED10_Pin;
  HAL_GPIO_Init(M_nLED10_Port, &GPIO_InitStruct);

  //------------ Keyboard -----------
  // Keyboard
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = M_KEY1_Pin;
  HAL_GPIO_Init(M_KEY1_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_KEY2_Pin;
  HAL_GPIO_Init(M_KEY2_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_KEY3_Pin;
  HAL_GPIO_Init(M_KEY3_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_KEY4_Pin;
  HAL_GPIO_Init(M_KEY4_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_KEY5_Pin;
  HAL_GPIO_Init(M_KEY5_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin   = M_KEY6_Pin;
  HAL_GPIO_Init(M_KEY6_Port, &GPIO_InitStruct);
}
//-----------------------------------------------------------------------------
