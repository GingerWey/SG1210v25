//-----------------------------------------------------------------------------
/*
 File        : main.c
 Version     : V1.10
 By          : 银网科技

 For         : Stm32
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
                
 Description :固件入口
 
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "gpio.h"
#include "iwdg.h"

#include "rtc.h"

#include "GUI.h"
#include "TaskApp.h"
#include "DevRegs.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局变量引用
//-----------------------------------------------------------------------------
// 中断向量表
extern void *__Vectors;
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
void EnableFlashReadProection()
{

  FLASH_OBProgramInitTypeDef OBInit;
  OBInit.RDPLevel = 0;
  HAL_FLASHEx_OBGetConfig( &OBInit );
  if( OB_RDP_LEVEL_1 != OBInit.RDPLevel )
    {
    HAL_FLASH_OB_Unlock();
            
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel   = OB_RDP_LEVEL_1;    
    HAL_FLASHEx_OBProgram( &OBInit );
    
    HAL_FLASH_OB_Launch();
    
    HAL_FLASH_OB_Lock();
      
    HAL_NVIC_SystemReset();
    }
}
//-----------------------------------------------------------------------------
/*
* System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Configure the main internal regulator output voltage
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  // Initializes the CPU, AHB and APB busses clocks
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | 
                                     //RCC_OSCILLATORTYPE_LSE |
                                     RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  // 25/25*336/2=168M
  // 25/25*336/7=48M
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
    SetHWFault( RHF_RCC_ERR );
    }

  // Initializes the CPU, AHB and APB busses clocks 
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
    SetHWFault( RHF_RCC_ERR );
    }

  HAL_SYSTICK_Config( HAL_RCC_GetHCLKFreq() / 1000 );

  HAL_SYSTICK_CLKSourceConfig( SYSTICK_CLKSOURCE_HCLK );

  // SysTick_IRQn interrupt configuration
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
int main(void)
{

  // 设置中断向量表的位置在 __Vectors表定义的位置
  SCB->VTOR = (int)&__Vectors;

  // 在调试状态下禁止IWD
//  __HAL_DBGMCU_FREEZE_WWDG();
  __HAL_DBGMCU_FREEZE_IWDG();

  // Reset of all peripherals, Initializes the Flash interface and the Systick.
  HAL_Init();

  
  // Configure the system clock
  SystemClock_Config();

  // EnableFlashReadProection();

#if osCMSIS >= 0x20000U
  // Initialize the RTOS Kernel.
  osKernelInitialize();
#endif

  // Call init function for freertos tasks 
  Tasks_Init();

  // Start scheduler
  osKernelStart();

  // Infinite loop
  while (1)
    {
    }
}
//-----------------------------------------------------------------------------
/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM9 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

  // USER CODE END Callback 0
  if (htim->Instance == TIM9)
    {
    // HAL时钟计数
    HAL_IncTick();
      
    SetRSTSrc( RRS_TMRTASK );

    // 复位看门狗
    constexpr auto WDG_FEED_SRC = (RRS_APPTASK | RRS_TMRTASK  | RRS_ADCTASK | 
                                   RRS_GUITASK | RRS_UARTTASK | RRS_CTRLTASK);
    auto Dogfeed = GetRSTSrc(WDG_FEED_SRC);
    // 各任务就绪?
    if( WDG_FEED_SRC  == Dogfeed )  
      {
      // 喂狗
      IWDG_FeedDog();

      // 清各任务标识
      ClrRSTSrc(WDG_FEED_SRC);
      }
#ifdef __DEBUG
    else
      RTC_WriteBkReg( 0, DevCache.DeviceState[REG_RST_SOURCE - REG_DEVSTATE] );
#endif
    }
}
//-----------------------------------------------------------------------------
/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
}
//-----------------------------------------------------------------------------
#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
}
#endif /* USE_FULL_ASSERT */
//-----------------------------------------------------------------------------

