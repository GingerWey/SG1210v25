//-----------------------------------------------------------------------------
/*
 File        : Flash_If.h
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f10x / Stm32F4xx
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description :定义片内Flash访问方法
        
 v1.0  2015.9.26
   完成STM32F4xx访问方法
   
 v2.0  2016.12.01
   增加对STM32F10x的Flash访问
*/
//-----------------------------------------------------------------------------
#ifndef __FLASH_IF_H
#define __FLASH_IF_H

#include <stdint.h>
/* Includes ------------------------------------------------------------------*/
#if defined(STM32F427xx) | defined(STM32F429xx) | defined(STM32F407xx)
  #include "stm32f4xx.h"
#elif defined(STM32F10xE)
  #include "stm32f4xx_hal.h"
#endif
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
// F4xx的Flash地址定义
#if defined(STM32F427xx) | defined(STM32F429xx)
  #define APPLICATION_ADDRESS     (uint32_t)0x08000000 

  #define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) // Base @ of Sector  0, 16 Kbyte
  #define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) // Base @ of Sector  1, 16 Kbyte
  #define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) // Base @ of Sector  2, 16 Kbyte
  #define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) // Base @ of Sector  3, 16 Kbyte
  #define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) // Base @ of Sector  4, 64 Kbyte
  #define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) // Base @ of Sector  5, 128 Kbyte
  #define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) // Base @ of Sector  6, 128 Kbyte
  #define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) // Base @ of Sector  7, 128 Kbyte
  #define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) // Base @ of Sector  8, 128 Kbyte
  #define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) // Base @ of Sector  9, 128 Kbyte 
  #define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) // Base @ of Sector 10, 128 Kbyte
  #define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) // Base @ of Sector 11, 128 Kbyte
  // End of the Flash address
  #define CODE_FLASH_END_ADDRESS  0x080FFFFF

  #define SIZE_ChipFlash          (*(uint16_t*)FLASHSIZE_BASE * 1024)

#elif defined(STM32F407xx)
  #define APPLICATION_ADDRESS     (uint32_t)0x08000000 

  #define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) // Base @ of Sector  0, 16 Kbyte
  #define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) // Base @ of Sector  1, 16 Kbyte
  #define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) // Base @ of Sector  2, 16 Kbyte
  #define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) // Base @ of Sector  3, 16 Kbyte
  #define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) // Base @ of Sector  4, 64 Kbyte
  #define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) // Base @ of Sector  5, 128 Kbyte
  #define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) // Base @ of Sector  6, 128 Kbyte
  #define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) // Base @ of Sector  7, 128 Kbyte
  // End of the Flash address
  #define CODE_FLASH_END_ADDRESS        0x0807FFFF
  
  #define SIZE_ChipFlash          (*(uint16_t*)FLASHSIZE_BASE * 1024)
  
#elif defined(STM32F103xE)
  #define APPLICATION_ADDRESS     0x08000000

  #define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) // Base @ of Sector 0, 2 Kbyte */
  #define FLASH_SECTOR_SIZE       ((uint32_t)FLASH_PAGE_SIZE) // 

  // End of the Flash address
  #define CODE_FLASH_END_ADDRESS  0x0807FFFF
#else
  #error Unknown MCU
#endif

// Define the user application size
#define APP_FLASH_SIZE   (SIZE_ChipFlash - (APPLICATION_ADDRESS - FLASH_BASE))
//=============================================================================
// 全局数据申明
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
void FLASH_If_Init(void);
uint32_t FLASH_If_Erase(uint32_t uNumBytes);
uint32_t FLASH_If_BlankCheck(uint32_t uNumBytes);
uint32_t FLASH_If_Write(volatile uint32_t  uFlashAddress, 
                           const uint32_t* puData, 
                                 uint32_t  uNumInt32s);
uint32_t FLASH_If_GetWriteProtectionStatus(void);
uint32_t FLASH_If_DisableWriteProtection(void);
uint32_t FLASH_If_EnableWriteProtection(void);
uint32_t FLASH_If_DisableReadProtection(void);
uint32_t FLASH_If_EnableReadProtection(void);
uint32_t FLASH_If_GetReadProtection(void);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif  // __FLASH_IF_H
/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
