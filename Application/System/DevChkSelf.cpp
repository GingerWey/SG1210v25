//-----------------------------------------------------------------------------
/*
 File        : DevChkSelf.c
 Version     : V1.10
 By          : 银网科技
 Description :装置自检
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "DevChkSelf.h"

#include "GUI.h"

#include "stm32f4xx_Hal.h"
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据申明
//-----------------------------------------------------------------------------


//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
unsigned int DChkSelf_Start(int iShow)
{

//  GUICommand( GCMD_SET_COLOR, (void*)GUI_LIGHTGREEN );
//  GUICommand( GCMD_DISPSTRING, (void*)"Reset:");
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_PINRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"Pin" );
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_PORRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"POR/PDR" );
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_SFTRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"Software" );
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_IWDGRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"IWDG" );
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_WWDGRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"WWDG" );
//  if( __HAL_RCC_GET_FLAG( RCC_FLAG_LPWRRST ) )
//    GUICommand( GCMD_DISPSTRING, (void*)"Low_Power" );
//    
//  GUICommand( GCMD_DISPSTRING, (void*)"\nSYSClk: " );
//  switch( __HAL_RCC_GET_SYSCLK_SOURCE() )
//  {
//    case RCC_SYSCLKSOURCE_STATUS_HSI:
//      GUICommand( GCMD_DISPSTRING, (void*)"HSI" );
//      break;
//    case RCC_SYSCLKSOURCE_STATUS_HSE:
//      GUICommand( GCMD_DISPSTRING, (void*)"HSE" );
//      break;
//    case RCC_SYSCLKSOURCE_STATUS_PLLCLK:
//      GUICommand( GCMD_DISPSTRING, (void*)"PLL" );
//      break;
//    default:
//      GUICommand( GCMD_SET_COLOR,  (void*)0x00FF8080 );
//      GUICommand( GCMD_DISPSTRING, (void*)"UNKN" );
//      GUICommand( GCMD_SET_COLOR,  (void*)GUI_LIGHTGREEN );
//      break;
//  }
//  GUICommand( GCMD_DISPUINT,     (void*)(HAL_RCC_GetSysClockFreq() / 1000000) );

////  GUICommand( GCMD_DISPSTRING, (void*)"M\nRTC Clock: " );
////  switch( __HAL_RCC_GET_RTC_SOURCE() )
////  {
////    case RCC_RTCCLKSOURCE_NO_CLK:
////      GUICommand( GCMD_DISPSTRING, (void*)"None" );
////      break;
////    case RCC_RTCCLKSOURCE_LSE:
////      GUICommand( GCMD_DISPSTRING, (void*)"LSE" );
////      break;
////    case RCC_RTCCLKSOURCE_LSI:
////      GUICommand( GCMD_DISPSTRING, (void*)"LSI" );
////      break;
////    default:
////       GUICommand( GCMD_SET_COLOR,  (void*)0x00FF8080 );
////      GUICommand( GCMD_DISPSTRING, (void*)"UNKN" );
////      GUICommand( GCMD_SET_COLOR, (void*)GUI_LIGHTGREEN );
////      break;
////  }
//  GUICommand( GCMD_DISPSTRING, (void*)"\n" );

  return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
