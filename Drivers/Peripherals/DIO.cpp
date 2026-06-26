//-----------------------------------------------------------------------------
/*
 File        : dio.h
 Version     : V1.10
 By          : 银网科技

 Description :数字输入输出信号检测
        
 Date       : 2023.12.05 2019.8.29
 
 2019.8
   移植到GPT30v23

 2024.1.19
   移植到SG1210v12 
*/
//-----------------------------------------------------------------------------
#include "dio.h"

#include "gpio.h"

#include "DevRegs.h"
#include "DevEvtMgr.h"
#include "DevDebug.h"

#include <string.h>
#include "stm32f4xx_hal.h"
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据申明
//-----------------------------------------------------------------------------
#ifdef YXs_State
  static uint16_t FDITimer[NUM_DigInputs];    // DI定时器
  static uint32_t FDIState;                   // DI状态
#endif
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 更新遥信状态寄存器
// 输入：
//   uChgMask: 按位描述的变位点
//      bInit: 是否是初始化过程，初始化过程中不发报警
static void _UpdateDIRegs(uint32_t uChgMask, bool bInit)
{

  // 扫描全部开入
  for( uint32_t uMask = 1, uIdx = 0; uIdx < NUM_DigInputs; uIdx++ )
    {
    // 是否有确认的变位
    if( 0 != (uChgMask & uMask) )
      {
      // 将确认的状态填入 硬遥信状态存器
      if( 0 != (FDIState & uMask) )
        {
        DevCache.IOState[REG_DigInp - REG_IOSTATE + uIdx] = STATE_TRUE;

        if( false == bInit )
          EVTMGR_AppendEvent( REG_DigInp + uIdx, STATE_TRUE );
        }
      else
        {
        DevCache.IOState[REG_DigInp - REG_IOSTATE + uIdx] = STATE_FALSE;

        if( false == bInit )
          EVTMGR_AppendEvent( REG_DigInp + uIdx, STATE_FALSE );
        }
      }

    uMask <<= 1;  
    }
}
//-----------------------------------------------------------------------------
static void _DISignalChanged(uint32_t uSignDiff)
{
}
//-----------------------------------------------------------------------------
// DIO重新赋值
static void DISCAN_Cover(void)
{

#ifdef YXs_State
  memset( FDITimer,  0, sizeof(FDITimer) );
  
  FDIState = YXs_State;
  
  _UpdateDIRegs( -1ul, true );
  
  _DISignalChanged( -1ul );
#endif
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// DIO初始化
void DISCAN_Init(void)
{

  DISCAN_Cover();
}
//-----------------------------------------------------------------------------
// 遥信扫描
void DISCAN_SignalScan(void)
{
  
#ifdef YXs_State
  uint32_t uSign = YXs_State;  
  uint32_t uDiff = uSign ^ FDIState, 
           uSignDiff = 0;

  // 可配置的消抖延时
  uint16_t uwDIJEDelay = MIN_DIJEDelay;
  
  // 逐个检查DI
  uint32_t uMask = 1;
  for( int iIdx = 0; iIdx < NUM_DigInputs; iIdx++ )
    {
    if( (uDiff & uMask) == uMask )
      {
      if( 0 == FDITimer[iIdx] )
        FDITimer[iIdx] = uwDIJEDelay;
      else
        {
        FDITimer[iIdx]--;
            
        if( 0 == FDITimer[iIdx] )
          {
          if( uMask == (uSign & uMask) )
            FDIState |= uMask;
          else
            FDIState &= (uint32_t)(~uMask);
          
          // 确认的变位
          uSignDiff |= uMask;
          }
        }
      }
    else if( FDITimer[iIdx] > 0 )
      {
      FDITimer[iIdx] = 0;
      }
    
    uMask <<= 1;
    }
    
  if( 0 != uSignDiff )
    {
    // 更新寄存器
    _UpdateDIRegs(  uSignDiff, false );  
      
    // 通知其它模块，有遥信变位
    _DISignalChanged( uSignDiff );
    }
#endif
}
//-----------------------------------------------------------------------------

