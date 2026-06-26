//-----------------------------------------------------------------------------
/*
 File        : KeyBoard.h
 Version     : V1.10
 By          : 银网科技

 Description :数字输入输出信号检测
        
 Date       : 2023.12.15
 
 2019.8
   移植到GPT30v23
*/
//-----------------------------------------------------------------------------
#include "KeyBoard.h"

#include "GUIConf.h"
#include "gpio.h"

#include <cmsis_os.h>
#include <string.h>
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
#define  NUM_KEYS                 6
#define  KEYS_DEJITTER_DELAY      50
//=============================================================================
// 局部数据申明
//-----------------------------------------------------------------------------
static uint32_t FKBDTimers[NUM_KEYS];    // KeyBoard定时器
static uint32_t FKeysPress[NUM_KEYS];    // Key按下的时刻
static uint16_t FuKeyState, FuKeyDown;
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 扫描按键
// 返回：哪些键状态有变化
static uint32_t _ScanKeyboard()
{
  
  uint32_t uKeyState = GetKeysStaue,
           uKeyDiff  = FuKeyState ^ uKeyState;
  
  uint32_t uKeysComfrim = 0;
  if( 0 != uKeyDiff )
    {
    uint32_t uNow = osKernelGetTickCount();
    for( int iIdx = 0; iIdx < NUM_KEYS; iIdx++ )
      {
      uint32_t uMask = 1 << iIdx;
      if( (uKeyDiff & uMask) == uMask )
        {
        // 按键状态有变化
        if( FKBDTimers[iIdx] > 0 )
          {
          // 消抖时间达到？
          if( KEYS_DEJITTER_DELAY < uNow - FKBDTimers[iIdx] )
            {
            // 确认按键状态有变化
            FKeysPress[iIdx] = uNow;
            FKBDTimers[iIdx] = 0;
              
            // 更新按键保存
            if( 0 == (uKeyState & uMask) )
              FuKeyState &= ~uMask;
            else
              FuKeyState |=  uMask;
            
            // 有按键变化被确认
            uKeysComfrim |= uMask;
            }
          }
        else
          // 开始消抖
          FKBDTimers[iIdx] = uNow;
        }
      else
        // 状态无变化，则清取抖计时
        FKBDTimers[iIdx] = 0;
      }
    }
    
  return uKeysComfrim;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// DIO初始化
void KeyBoard_Init(void)
{

  memset( FKBDTimers, 0, sizeof(FKBDTimers) );
  memset( FKeysPress, 0, sizeof(FKeysPress) );
  
  FuKeyState = GetKeysStaue;
  FuKeyDown  = FuKeyState;
}
//-----------------------------------------------------------------------------
// 按键定时扫描
// 返回：有几哪个按键信息
uint32_t KeyBoard_Tick(void)
{
  
  // 变化的，按下的，都算
  return _ScanKeyboard() | FuKeyState;
}
//-----------------------------------------------------------------------------
// 读回按键信息
// 输入：pKeys 存贮按键信息的空间
// 返回：无
void KeyBoard_Read(TKeyState &Key)
{
  
  constexpr uint16_t uKeyCodes[] = { KEY_UP, 
                                     KEY_RIGHT, KEY_ENTER, KEY_LEFT,
                                     KEY_DOWN,  KEY_ESCAPE }; 
  
  Key.ucKey = 0;
  uint32_t uNow = osKernelGetTickCount();
  for( int iIdx = 0; iIdx < NUM_KEYS; iIdx++ )
    {
    uint32_t uMask = 1 << iIdx;
    if( 0 < FKeysPress[iIdx] )
      {
      Key.ucKey = uKeyCodes[iIdx];
      Key.uwPressCnt = uNow - FKeysPress[iIdx];

      // 有键刚被按下
      if( 0 != (FuKeyState & uMask) )
        {
        // 按键动作的性质
        if( 0 == (FuKeyDown & uMask) )
          {
          Key.ucState = KEYST_DOWN;
            
          FuKeyDown |= uMask;
          }
        else
          {
          Key.ucState = KEYST_PRESS;
          }
        }
      else
        {
        Key.ucState = KEYST_RELEASE;
        FKeysPress[iIdx] = 0;
          
        FuKeyDown &= ~uMask;
        }
      }
    
    if( 0 != Key.ucKey )
      break;
    }
}
//-----------------------------------------------------------------------------
