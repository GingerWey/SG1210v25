//-----------------------------------------------------------------------------
/*
   File        : gpio25.h
   Version     : V2.50
   By          : 银网科技


   Description : 定义MCU功能管脚

   Date        : 2025.6.02

   v1.1
     配置HWv1.1
   v2.1
     配置HWv2.0
   v2.5
     配置HWv2.5
*/
//-----------------------------------------------------------------------------
#ifndef _GPIO25_H
#define _GPIO25_H

#include "stm32f4xx_hal.h"
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
#define SetPinToHigh(port, pin)  (port->BSRR = (pin))
#define SetPinToLow(port, pin)   (port->BSRR = ((uint32_t)(pin) << 16))
//-----------------------------------------------------------------------------
//------------ 模拟量输入信号 -----------
// ADC123_10
// 交流输入
#define AVin0_Pin           GPIO_PIN_0
#define AVin0_Port          GPIOC
// ADC123_11
// 逆变器输出
#define AVin1_Pin           GPIO_PIN_1
#define AVin1_Port          GPIOC
// ADC123_12
// 温度输入1
// 充电器温度
#define ATin0_Pin           GPIO_PIN_2
#define ATin0_Port          GPIOC
// ADC123_0
// 温度输入2
// v2.5新增, 电池温度
#define ATin1_Pin           GPIO_PIN_0
#define ATin1_Port          GPIOA

// 交流输入频率
#define Freq0_Pin           GPIO_PIN_10
#define Freq0_Port          GPIOB
// 逆变器输出频率
#define Freq1_Pin           GPIO_PIN_11
#define Freq1_Port          GPIOB

//------------ 控制信号 -----------
// MCU Power Keep ON
// 2024/10/2 ok
#define KeepPowON_Pin       GPIO_PIN_12
#define KeepPowON_Port      GPIOC
// 正逻辑
#define KeepMCUPower_ON     SetPinToHigh(KeepPowON_Port, KeepPowON_Pin)
#define KeepMCUPower_OFF    SetPinToLow (KeepPowON_Port, KeepPowON_Pin)
#define KeepMCUPower_State  ((KeepPowON_Port->ODR & KeepPowON_Pin)? 1 : 0)

// 对外脱扣器供电模式
// AC Output mode control
// SG121B20v2.5 
#define ACout_nPassby_Pin   GPIO_PIN_12
#define ACout_nPassby_Port  GPIOA
// 低电平Passby，高电平Invertor
#define ACout_Passby_ON     SetPinToLow ( ACout_nPassby_Port, ACout_nPassby_Pin )
#define ACout_Passby_OFF    SetPinToHigh( ACout_nPassby_Port, ACout_nPassby_Pin )
#define ACout_Passby_State  ((ACout_nPassby_nPort->IDR & ACout_nPassby_Pin)? 0 : 1)

// 逆变器工作允许控制
// Invertor enable control
// SG121B20v2.5 
#define Invertor_Pwr_Pin    GPIO_PIN_11
#define Invertor_Pwr_Port   GPIOA
// 正逻辑
#define Invertor_ON         SetPinToHigh( Invertor_Pwr_Port, Invertor_Pwr_Pin )
#define Invertor_OFF        SetPinToLow ( Invertor_Pwr_Port, Invertor_Pwr_Pin )
#define Invertor_State      ((Invertor_Pwr_Port->ODR & Invertor_Pwr_Pin)? 1 : 0)

// 风扇控制
// Device Fan control
// v2.5取消380EN,加FanEN
#define DevFanCtrl_Pin      GPIO_PIN_8
#define DevFanCtrl_Port     GPIOC
// 正逻辑
#define DevFanCtrl_ON       SetPinToHigh( DevFanCtrl_Port, DevFanCtrl_Pin )
#define DevFanCtrl_OFF      SetPinToLow ( DevFanCtrl_Port, DevFanCtrl_Pin )
#define DevFanCtrl_State    ((DevFanCtrl_Port->ODR & DevFanCtrl_Pin)? 1 : 0)

// 合闸继电器控制
// Trip Relay control
// 2024/10/2 ok
#define Trip1_Pin           GPIO_PIN_9
#define Trip1_Port          GPIOC
// 正逻辑                   
#define Trip1_ON            SetPinToHigh( Trip1_Port, Trip1_Pin )
#define Trip1_OFF           SetPinToLow ( Trip1_Port, Trip1_Pin )
#define Trip1_State         ((Trip1_Port->ODR & Trip1_Pin)? 1 : 0)

// 电池加热器器控制
// Battery heater control
// v2.5 与Adapter_ON交换
#define Heater_Pin          GPIO_PIN_7
#define Heater_Port         GPIOC
// 正逻辑                   
#define Heater_ON           SetPinToHigh( Heater_Port, Heater_Pin )
#define Heater_OFF          SetPinToLow ( Heater_Port, Heater_Pin )
#define Heater_State        ((Heater_Port->ODR & Heater_Pin)? 1 : 0)

//------------ 状态信号 -----------
// 开关电源输出有效状态
// Adapter output avalidable 
// v2.5 与Heater交换
#define Adapter_ON_Pin      GPIO_PIN_6
#define Adapter_ON_Port     GPIOC
// 正逻辑
#define Adapter_ON_State    ((Adapter_ON_Port->IDR & Adapter_ON_Pin)? 1 : 0)

// 逆变器辅助电源，即18V输出
// SG121B20v2.5 
#define Invertor5V_Pin      GPIO_PIN_10
#define Invertor5V_Port     GPIOC
// 正逻辑
#define Invertor5V_State    ((Invertor5V_Port->IDR & Invertor5V_Pin)? 1 : 0)

//------------ 读状态信号 -----------
// 读状态信号
// 2024/10/2 ok
#define YXs_State           ( (KeepMCUPower_State  << 0) | \
                              (Adapter_ON_State    << 1) | \
                              (Invertor5V_State    << 2) | \
                              (Invertor_State      << 3) | \
                              (Trip1_State         << 5) | \
                              (ACout_Passby_State  << 4) | \
                              (Heater_State        << 6) | \
                              (DevFanCtrl_State    << 7) )

//------------ RTC -----------
// RTC I2C bus
// 2024/10/2 ok
#define RTC_SCL_Pin         GPIO_PIN_1
#define RTC_SCL_Port        GPIOA
#define RTC_SCL_HIGH        SetPinToHigh( RTC_SCL_Port, RTC_SCL_Pin )
#define RTC_SCL_LOW         SetPinToLow ( RTC_SCL_Port, RTC_SCL_Pin )

#define RTC_SDA_Pin         GPIO_PIN_2
#define RTC_SDA_Port        GPIOA
#define RTC_SDA_HIGH        SetPinToHigh( RTC_SDA_Port, RTC_SDA_Pin )
#define RTC_SDA_LOW         SetPinToLow ( RTC_SDA_Port, RTC_SDA_Pin )
#define RTC_SDA_Input       ((RTC_SDA_Port->IDR & RTC_SDA_Pin)? 1 : 0)

//------------ 功率板I2C -----------
// Power board IIC
// SG121B20v2.5 
#define SI2C_SCL_Pin        GPIO_PIN_15
#define SI2C_SCL_Port       GPIOA
#define SI2C_SCL_HIGH       SetPinToHigh( SI2C_SCL_Port, SI2C_SCL_Pin )
#define SI2C_SCL_LOW        SetPinToLow ( SI2C_SCL_Port, SI2C_SCL_Pin )

#define SI2C_SDA_Pin        GPIO_PIN_11
#define SI2C_SDA_Port       GPIOC
#define SI2C_SDA_HIGH       SetPinToHigh( SI2C_SDA_Port, SI2C_SDA_Pin )
#define SI2C_SDA_LOW        SetPinToLow ( SI2C_SDA_Port, SI2C_SDA_Pin )
#define SI2C_SDA_Input      ((SI2C_SDA_Port->IDR & SI2C_SDA_Pin)? 1 : 0)

//------------ NVRAM -----------
// NVRAM SPI bus
// SG121B20v2.5 
#define NvRAM_SCLK_Pin      GPIO_PIN_3
#define NvRAM_SCLK_Port     GPIOB
#define NvRAM_MISO_Pin      GPIO_PIN_4
#define NvRAM_MISO_Port     GPIOB
#define NvRAM_MISO_Input    ((NvRAM_MISO_Port->IDR & NvRAM_MISO_Pin)? 1 : 0)
#define NvRAM_MOSI_Pin      GPIO_PIN_5
#define NvRAM_MOSI_Port     GPIOB

// FeRAM
// 2024/10/2 ok
#define FeRAM_nCS_Pin       GPIO_PIN_6
#define FeRAM_nCS_Port      GPIOD
#define FeRAM_nCS_HIGH      SetPinToHigh( FeRAM_nCS_Port, FeRAM_nCS_Pin )
#define FeRAM_nCS_LOW       SetPinToLow ( FeRAM_nCS_Port, FeRAM_nCS_Pin )

#define FeRAM_nWP_Pin       GPIO_PIN_7
#define FeRAM_nWP_Port      GPIOD
#define FeRAM_nWP_HIGH      SetPinToHigh( FeRAM_nWP_Port, FeRAM_nWP_Pin )
#define FeRAM_nWP_LOW       SetPinToLow ( FeRAM_nWP_Port, FeRAM_nWP_Pin )

// SPI Flash
// 2024/10/2 ok
#define FLASH_nCS_Pin       GPIO_PIN_2
#define FLASH_nCS_Port      GPIOD
#define FLASH_nCS_HIGH      SetPinToHigh( FLASH_nCS_Port, FLASH_nCS_Pin )
#define FLASH_nCS_LOW       SetPinToLow ( FLASH_nCS_Port, FLASH_nCS_Pin )

#define FLASH_nWP_Pin       GPIO_PIN_3
#define FLASH_nWP_Port      GPIOD
#define FLASH_nWP_HIGH      SetPinToHigh( FLASH_nWP_Port, FLASH_nWP_Pin )
#define FLASH_nWP_LOW       SetPinToLow ( FLASH_nWP_Port, FLASH_nWP_Pin )

//------------ LCD -----------
// Reset
// 2024/10/2 ok
#define LCD_nRST_Pin        GPIO_PIN_2
#define LCD_nRST_Port       GPIOB
#define LCD_nRST_HIGH       SetPinToHigh( LCD_nRST_Port, LCD_nRST_Pin )
#define LCD_nRST_LOW        SetPinToLow ( LCD_nRST_Port, LCD_nRST_Pin )

// Lamp
// 2024/10/2 ok
#define LCD_nLamp_Pin       GPIO_PIN_1
#define LCD_nLamp_Port      GPIOB
// 负逻辑
#define LCD_Lamp_ON         SetPinToLow ( LCD_nLamp_Port, LCD_nLamp_Pin )
#define LCD_Lamp_OFF        SetPinToHigh( LCD_nLamp_Port, LCD_nLamp_Pin )
#define LCD_Lamp_State      ((LCD_nLamp_Port->ODR & LCD_nLamp_Pin)? 0 : 1)

// LCD nCS在地址线中
//#define LCD_nCS_Pin         GPIO_PIN_11
//#define LCD_nCS_Port        GPIOD
//#define LCD_nCS_HIGH        SetPinToHigh( LCD_nCS_Port, LCD_nCS_Pin )
//#define LCD_nCS_LOW         SetPinToLow ( LCD_nCS_Port, LCD_nCS_Pin )

// LCD RS在地址线中
//#define LCD_RS_Pin          GPIO_PIN_12
//#define LCD_RS_Port         GPIOD
//#define LCD_RS_HIGH         SetPinToHigh( LCD_RS_Port, LCD_RS_Pin )
//#define LCD_RS_LOW          SetPinToLow ( LCD_RS_Port, LCD_RS_Pin )

//------------ USART1 -----------
// USART1
// 2024/10/2 ok
#define USART1_TXD_Pin      GPIO_PIN_9
#define USART1_TXD_Port     GPIOA
#define USART1_RXD_Pin      GPIO_PIN_10
#define USART1_RXD_Port     GPIOA

// USART1 Tx enable
#define USART1_TxEn_Pin     GPIO_PIN_8
#define USART1_TxEn_Port    GPIOA
#define USART1_TxEn_ON      SetPinToHigh( USART1_TxEn_Port, USART1_TxEn_Pin )
#define USART1_TxEn_OFF     SetPinToLow ( USART1_TxEn_Port, USART1_TxEn_Pin )

//------------ LEDs -----------
// Status LEDs
// 2024/10/2 ok
#define M_nLED1_Pin         GPIO_PIN_0
#define M_nLED1_Port        GPIOB
#define LED1_ON             SetPinToLow ( M_nLED1_Port, M_nLED1_Pin )
#define LED1_OFF            SetPinToHigh( M_nLED1_Port, M_nLED1_Pin )
  
#define M_nLED2_Pin         GPIO_PIN_5
#define M_nLED2_Port        GPIOC
#define LED2_ON             SetPinToLow ( M_nLED2_Port, M_nLED2_Pin )
#define LED2_OFF            SetPinToHigh( M_nLED2_Port, M_nLED2_Pin )

#define M_nLED3_Pin         GPIO_PIN_4
#define M_nLED3_Port        GPIOC
#define LED3_ON             SetPinToLow ( M_nLED3_Port, M_nLED3_Pin )
#define LED3_OFF            SetPinToHigh( M_nLED3_Port, M_nLED3_Pin )
  
#define M_nLED4_Pin         GPIO_PIN_7
#define M_nLED4_Port        GPIOA
#define LED4_ON             SetPinToLow ( M_nLED4_Port, M_nLED4_Pin )
#define LED4_OFF            SetPinToHigh( M_nLED4_Port, M_nLED4_Pin )

#define M_nLED5_Pin         GPIO_PIN_6
#define M_nLED5_Port        GPIOA
#define LED5_ON             SetPinToLow ( M_nLED5_Port, M_nLED5_Pin )
#define LED5_OFF            SetPinToHigh( M_nLED5_Port, M_nLED5_Pin )

#define M_nLED6_Pin         GPIO_PIN_5
#define M_nLED6_Port        GPIOA
#define LED6_ON             SetPinToLow ( M_nLED6_Port, M_nLED6_Pin )
#define LED6_OFF            SetPinToHigh( M_nLED6_Port, M_nLED6_Pin )

#define M_nLED7_Pin         GPIO_PIN_4
#define M_nLED7_Port        GPIOA
#define LED7_ON             SetPinToLow ( M_nLED7_Port, M_nLED7_Pin )
#define LED7_OFF            SetPinToHigh( M_nLED7_Port, M_nLED7_Pin )

#define M_nLED8_Pin         GPIO_PIN_6
#define M_nLED8_Port        GPIOE
#define LED8_ON             SetPinToLow ( M_nLED8_Port, M_nLED8_Pin )
#define LED8_OFF            SetPinToHigh( M_nLED8_Port, M_nLED8_Pin )

#define M_nLED9_Pin         GPIO_PIN_5
#define M_nLED9_Port        GPIOE
#define LED9_ON             SetPinToLow ( M_nLED9_Port, M_nLED9_Pin )
#define LED9_OFF            SetPinToHigh( M_nLED9_Port, M_nLED9_Pin )

#define M_nLED10_Pin        GPIO_PIN_4
#define M_nLED10_Port       GPIOE
#define LED10_ON            SetPinToLow ( M_nLED10_Port, M_nLED10_Pin )
#define LED10_OFF           SetPinToHigh( M_nLED10_Port, M_nLED10_Pin )

//------------ Keyboard -----------
// Keyboard 负逻辑
// 2024/10/2 ok
#define M_KEY1_Pin          GPIO_PIN_8  // KEY_UP
#define M_KEY1_Port         GPIOB
#define KEY1_State          ((M_KEY1_Port->IDR & M_KEY1_Pin)? 0 : 1)
  
#define M_KEY2_Pin          GPIO_PIN_9  // KEY_RIGHT
#define M_KEY2_Port         GPIOB
#define KEY2_State          ((M_KEY2_Port->IDR & M_KEY2_Pin)? 0 : 2)

#define M_KEY3_Pin          GPIO_PIN_0  // KEY_ENTER
#define M_KEY3_Port         GPIOE
#define KEY3_State          ((M_KEY3_Port->IDR & M_KEY3_Pin)? 0 : 4)
  
#define M_KEY4_Pin          GPIO_PIN_1  // KEY_LEFT
#define M_KEY4_Port         GPIOE
#define KEY4_State          ((M_KEY4_Port->IDR & M_KEY4_Pin)? 0 : 8)

#define M_KEY5_Pin          GPIO_PIN_3  // KEY_DOWN
#define M_KEY5_Port         GPIOE
#define KEY5_State          ((M_KEY5_Port->IDR & M_KEY5_Pin)? 0 : 16)

#define M_KEY6_Pin          GPIO_PIN_2  // KEY_ESC
#define M_KEY6_Port         GPIOE
#define KEY6_State          ((M_KEY6_Port->IDR & M_KEY6_Pin)? 0 : 32)

#define GetKeysStaue        (KEY1_State | KEY2_State | KEY3_State | \
                             KEY4_State | KEY5_State | KEY6_State)
//-----------------------------------------------------------------------------
#endif
