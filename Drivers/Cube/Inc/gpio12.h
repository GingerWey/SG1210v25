//-----------------------------------------------------------------------------
/*
   File        : gpio12.h
   Version     : V1.10
   By          : 银网科技


   Description : 定义MCU功能管脚

   Date        : 2023.12.05

   v1.1
     配置HWv1.2
*/
//-----------------------------------------------------------------------------
#ifndef _GPIO12_H
#define _GPIO12_H

#include "stm32f4xx_hal.h"
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
#define SetPinToHigh(port, pin)  (port->BSRR = (pin))
#define SetPinToLow(port, pin)   (port->BSRR = ((uint32_t)(pin) << 16))
//-----------------------------------------------------------------------------
// ADC123_10
#define AVin0_Pin           GPIO_PIN_0
#define AVin0_Port          GPIOC

// MCU Power Keep ON
#define KeepPowON_Pin       GPIO_PIN_12
#define KeepPowON_Port      GPIOB
#define KeepMCUPower_ON     SetPinToHigh(KeepPowON_Port, KeepPowON_Pin)
#define KeepMCUPower_OFF    SetPinToLow (KeepPowON_Port, KeepPowON_Pin)
#define KeepMCUPower_State  ((KeepPowON_Port->ODR & KeepPowON_Pin)? 1 : 0)

// AC input avalidable 
#define AC_ON_Pin           GPIO_PIN_12
#define AC_ON_Port          GPIOA
#define AC_ON_State         ((AC_ON_Port->IDR & AC_ON_Pin)? 1 : 0)

// Coils keep power supply control
#define Coils_PowON_Pin     GPIO_PIN_8
#define Coils_PowON_Port    GPIOC
// 高电平有效
#define Coils_Power_ON      SetPinToHigh( Coils_PowON_Port, Coils_PowON_Pin )
#define Coils_Power_OFF     SetPinToLow ( Coils_PowON_Port, Coils_PowON_Pin )
#define Coils_Power_State   ((Coils_PowON_Port->IDR & Coils_PowON_Pin)? 1 : 0)

// CoilA Relay control
#define CoilA_Trip_Pin      GPIO_PIN_9
#define CoilA_Trip_Port     GPIOC
// 正逻辑
#define CoilA_Trip_ON       SetPinToHigh( CoilA_Trip_Port, CoilA_Trip_Pin )
#define CoilA_Trip_OFF      SetPinToLow ( CoilA_Trip_Port, CoilA_Trip_Pin )
#define CoilA_Trip_State    ((CoilA_Trip_Port->ODR & CoilA_Trip_Pin)? 1 : 0)

// CoilB Relay control
#define CoilB_Trip_Pin      GPIO_PIN_8
#define CoilB_Trip_Port     GPIOA
// 正逻辑
#define CoilB_Trip_ON       SetPinToHigh( CoilB_Trip_Port, CoilB_Trip_Pin )
#define CoilB_Trip_OFF      SetPinToLow ( CoilB_Trip_Port, CoilB_Trip_Pin )
#define CoilB_Trip_State    ((CoilB_Trip_Port->ODR & CoilB_Trip_Pin)? 1 : 0)

#define YXs_State           ( (AC_ON_State)             | \
                              (KeepMCUPower_State << 1) | \
                              (Coils_Power_State  << 2) | \
                              (CoilA_Trip_State   << 3) | \
                              (CoilB_Trip_State   << 4) )

// RTC
#define RTC_SCL_Pin         GPIO_PIN_1
#define RTC_SCL_Port        GPIOA
#define RTC_SCL_HIGH        SetPinToHigh( RTC_SCL_Port, RTC_SCL_Pin )
#define RTC_SCL_LOW         SetPinToLow ( RTC_SCL_Port, RTC_SCL_Pin )

#define RTC_SDA_Pin         GPIO_PIN_2
#define RTC_SDA_Port        GPIOA
#define RTC_SDA_HIGH        SetPinToHigh( RTC_SDA_Port, RTC_SDA_Pin )
#define RTC_SDA_LOW         SetPinToLow ( RTC_SDA_Port, RTC_SDA_Pin )
#define RTC_SDA_Input       ((RTC_SDA_Port->IDR & RTC_SDA_Pin)? 1 : 0)

// Slave board IIC
#define SI2C_SCL_Pin        GPIO_PIN_7
#define SI2C_SCL_Port       GPIOC
#define SI2C_SCL_HIGH       SetPinToHigh( SI2C_SCL_Port, SI2C_SCL_Pin )
#define SI2C_SCL_LOW        SetPinToLow ( SI2C_SCL_Port, SI2C_SCL_Pin )

#define SI2C_SDA_Pin        GPIO_PIN_6
#define SI2C_SDA_Port       GPIOC
#define SI2C_SDA_HIGH       SetPinToHigh( SI2C_SDA_Port, SI2C_SDA_Pin )
#define SI2C_SDA_LOW        SetPinToLow ( SI2C_SDA_Port, SI2C_SDA_Pin )
#define SI2C_SDA_Input      ((SI2C_SDA_Port->IDR & SI2C_SDA_Pin)? 1 : 0)

// NVRAM
#define NvRAM_SCLK_Pin      GPIO_PIN_3
#define NvRAM_SCLK_Port     GPIOB
#define NvRAM_MISO_Pin      GPIO_PIN_4
#define NvRAM_MISO_Port     GPIOB
#define NvRAM_MISO_Input    ((NvRAM_MISO_Port->IDR & NvRAM_MISO_Pin)? 1 : 0)
#define NvRAM_MOSI_Pin      GPIO_PIN_5
#define NvRAM_MOSI_Port     GPIOB

// FeRAM
#define FeRAM_nWP_Pin       GPIO_PIN_6
#define FeRAM_nWP_Port      GPIOB
#define FeRAM_nWP_HIGH      SetPinToHigh( FeRAM_nWP_Port, FeRAM_nWP_Pin )
#define FeRAM_nWP_LOW       SetPinToLow ( FeRAM_nWP_Port, FeRAM_nWP_Pin )

#define FeRAM_nCS_Pin       GPIO_PIN_7
#define FeRAM_nCS_Port      GPIOB
#define FeRAM_nCS_HIGH      SetPinToHigh( FeRAM_nCS_Port, FeRAM_nCS_Pin )
#define FeRAM_nCS_LOW       SetPinToLow ( FeRAM_nCS_Port, FeRAM_nCS_Pin )

// SPI Flash
#define FLASH_nCS_Pin       GPIO_PIN_2
#define FLASH_nCS_Port      GPIOD
#define FLASH_nCS_HIGH      SetPinToHigh( FLASH_nCS_Port, FLASH_nCS_Pin )
#define FLASH_nCS_LOW       SetPinToLow ( FLASH_nCS_Port, FLASH_nCS_Pin )

#define FLASH_nWP_Pin       GPIO_PIN_3
#define FLASH_nWP_Port      GPIOD
#define FLASH_nWP_HIGH      SetPinToHigh( FLASH_nWP_Port, FLASH_nWP_Pin )
#define FLASH_nWP_LOW       SetPinToLow ( FLASH_nWP_Port, FLASH_nWP_Pin )

// LCD 
#define LCD_nLamp_Pin       GPIO_PIN_3
#define LCD_nLamp_Port      GPIOA
// 负逻辑
#define LCD_Lamp_ON         SetPinToLow ( LCD_nLamp_Port, LCD_nLamp_Pin )
#define LCD_Lamp_OFF        SetPinToHigh( LCD_nLamp_Port, LCD_nLamp_Pin )
#define LCD_Lamp_State      ((LCD_nLamp_Port->ODR & LCD_nLamp_Pin)? 0 : 1)

#define LCD_nRST_Pin        GPIO_PIN_2
#define LCD_nRST_Port       GPIOB
#define LCD_nRST_HIGH       SetPinToHigh( LCD_nRST_Port, LCD_nRST_Pin )
#define LCD_nRST_LOW        SetPinToLow ( LCD_nRST_Port, LCD_nRST_Pin )

//#define LCD_nCS_Pin         GPIO_PIN_11
//#define LCD_nCS_Port        GPIOD
//#define LCD_nCS_HIGH        SetPinToHigh( LCD_nCS_Port, LCD_nCS_Pin )
//#define LCD_nCS_LOW         SetPinToLow ( LCD_nCS_Port, LCD_nCS_Pin )

//#define LCD_RS_Pin          GPIO_PIN_12
//#define LCD_RS_Port         GPIOD
//#define LCD_RS_HIGH         SetPinToHigh( LCD_RS_Port, LCD_RS_Pin )
//#define LCD_RS_LOW          SetPinToLow ( LCD_RS_Port, LCD_RS_Pin )

// USART1
#define USART1_TXD_Pin      GPIO_PIN_9
#define USART1_TXD_Port     GPIOA
#define USART1_RXD_Pin      GPIO_PIN_10
#define USART1_RXD_Port     GPIOA

// USART1 Tx enable
#define USART1_TxEn_Pin     GPIO_PIN_11
#define USART1_TxEn_Port    GPIOA
#define USART1_TxEn_ON      SetPinToHigh( USART1_TxEn_Port, USART1_TxEn_Pin )
#define USART1_TxEn_OFF     SetPinToLow ( USART1_TxEn_Port, USART1_TxEn_Pin )

// Status LEDs
#define LED_nSTA_Pin        GPIO_PIN_3
#define LED_nSTA_Port       GPIOE
#define LED_STA_ON          SetPinToLow ( LED_nSTA_Port, LED_nSTA_Pin )
#define LED_STA_OFF         SetPinToHigh( LED_nSTA_Port, LED_nSTA_Pin )

#define LED_nSTB_Pin        GPIO_PIN_4
#define LED_nSTB_Port       GPIOE
#define LED_STB_ON          SetPinToLow ( LED_nSTB_Port, LED_nSTB_Pin )
#define LED_STB_OFF         SetPinToHigh( LED_nSTB_Port, LED_nSTB_Pin )
#define LED_STB_TOGGLE      { LED_nSTB_Port->ODR ^= LED_nSTB_Pin; }
  
#define M_nLED1_Pin         GPIO_PIN_15
#define M_nLED1_Port        GPIOA
#define LED1_ON             SetPinToLow ( M_nLED1_Port, M_nLED1_Pin )
#define LED1_OFF            SetPinToHigh( M_nLED1_Port, M_nLED1_Pin )

#define M_nLED2_Pin         GPIO_PIN_10
#define M_nLED2_Port        GPIOC
#define LED2_ON             SetPinToLow ( M_nLED2_Port, M_nLED2_Pin )
#define LED2_OFF            SetPinToHigh( M_nLED2_Port, M_nLED2_Pin )

#define M_nLED3_Pin         GPIO_PIN_11
#define M_nLED3_Port        GPIOC
#define LED3_ON             SetPinToLow ( M_nLED3_Port, M_nLED3_Pin )
#define LED3_OFF            SetPinToHigh( M_nLED3_Port, M_nLED3_Pin )
  
#define M_nLED4_Pin         GPIO_PIN_12
#define M_nLED4_Port        GPIOC
#define LED4_ON             SetPinToLow ( M_nLED4_Port, M_nLED4_Pin )
#define LED4_OFF            SetPinToHigh( M_nLED4_Port, M_nLED4_Pin )

// Keyboard 
#define M_KEY1_Pin          GPIO_PIN_8
#define M_KEY1_Port         GPIOB
#define KEY1_State          ((M_KEY1_Port->IDR & M_KEY1_Pin)? 0 : 1)
  
#define M_KEY2_Pin          GPIO_PIN_9
#define M_KEY2_Port         GPIOB
#define KEY2_State          ((M_KEY2_Port->IDR & M_KEY2_Pin)? 0 : 2)

#define M_KEY3_Pin          GPIO_PIN_0
#define M_KEY3_Port         GPIOE
#define KEY3_State          ((M_KEY3_Port->IDR & M_KEY3_Pin)? 0 : 4)
  
#define M_KEY4_Pin          GPIO_PIN_1
#define M_KEY4_Port         GPIOE
#define KEY4_State          ((M_KEY4_Port->IDR & M_KEY4_Pin)? 0 : 8)

#define M_KEY5_Pin          GPIO_PIN_2
#define M_KEY5_Port         GPIOE
#define KEY5_State          ((M_KEY5_Port->IDR & M_KEY5_Pin)? 0 : 16)

#define GetKeysStaue        (KEY1_State | KEY2_State | KEY3_State | \
                             KEY4_State | KEY5_State)
//-----------------------------------------------------------------------------
#endif
