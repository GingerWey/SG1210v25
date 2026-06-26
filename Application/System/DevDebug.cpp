//-----------------------------------------------------------------------------
/*
 File        : DevDebug.c
 Version     : V1.01
 By          : 银网科技

 Description :实现一些调试用的方法

   银网科技
   2017.9.12
*/
//-----------------------------------------------------------------------------
#include "DevDebug.h"

#include "DevBuffer.h"

#include "GUICntr.h"

#include "rtc.h"
#include "iwdg.h"
#include "IndLed.h"

#include <stdio.h>

#include "Strings/TextStrs.h"

#include <stm32f4xx_hal.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define GFC_SystemErr   0
#define GFC_OutOfMem    1
#define GFC_EmptyPtr    2
#define GFC_DevType     3
#define GFC_FunType     4
#define GFC_ErrParam    5
#define GFC_ErrRegNum   6
#define GFC_ErrToken    7
#define GFC_ErrValue    8
#define GFC_HardFault   9

constexpr const uint32_t FFatalStringIDs[] =
{
  idDevFault01,   // 0
  idDevFault02,   // 1
  idDevFault03,   // 2
  idDevFault04,   // 3
  idDevFault05,   // 4
  idDevFault06,   // 5
  idDevFault07,   // 6
  idDevFault08,   // 7
  idDevFault09,   // 8
  idDevFault10    // 9
};
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据引用
//-----------------------------------------------------------------------------
// GUI的桌面对象 
//extern TGUIDesktop FGUIDesktop;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#ifdef USE_DEV_ASSERT
// 报告出错信息，并锁死装置
int _Error_Handler(uint32_t uErrType, const char* pFileName, uint32_t uLineNo)
{

  // 禁止全部可屏蔽中断
  __disable_irq();

  uint32_t uPtr = 0, uPos = 0, uStart = 0;
  
  // 找到文件名 
//  while( pFileName[uPtr] ) uPtr++;
//  while( pFileName[uPtr] != '\\' && pFileName[uPtr] != '/' && uPtr > 0 ) uPtr--;
//  if( pFileName[uPtr] == '\\' || pFileName[uPtr] == '/' )
//    uPtr++;
  const char* pName = pFileName;
  while( *pName )
    {
    if( '\\' == *pName || '/' == *pName )
      uStart = uPtr + 1;
    
    uPtr++;
    pName++;
    }
  
  char pcBuf[128];
  uPos += sprintf( pcBuf + uPos, "Error no: %s\n", 
                                 GetMultiLangString(FFatalStringIDs[uErrType]) );
  uPos += sprintf( pcBuf + uPos, "Location:\n   File: %s\n   Line: %d\n", 
                                 pFileName + uStart, uLineNo );
  uPos += sprintf( pcBuf + uPos, "\n====================\n" );
  
  TDateTimeType dtNow;
  RTC_GetTime( &dtNow );
  uPos += sprintf( pcBuf + uPos, "%04u-%02u-%02u %02u:%02u:%02u\n", 
           dtNow.Year, 
           dtNow.Month, 
           dtNow.Day, 
           dtNow.Hours, 
           dtNow.Minutes, 
           dtNow.Seconds );
  
  GUIShowFatalMessage( pcBuf );
  
  INDLED_LCDBG( 1 );

  // 锁死
  while( 1 )
    {
    IWDG_FeedDog();
    }
    
//  return 1;
}
#endif
//=============================================================================

