//-----------------------------------------------------------------------------
/*
   中值平均滤波器

   银网科技
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "MidAvgFilter.h"

#include <string.h>

#include "Dev_Cfg.h"
#include "DevTypes.h"
#include "DevDebug.h"
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//  滤波器深度
#define NUM_FILTERDEEP    6u
//-----------------------------------------------------------------------------
// 宏有效性检查
#if NUM_FILTERDEEP < 3
#error  "The deepth of Middle_Average_Filter(NUM_FILTERDEEP) is shorter than 3."
#error  "You must be increase the NUM_FILTERDEEP."
#endif
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 滤波器数据
typedef __PACKED_BEG struct tagMiddleAvgFloatFilter
{
  uint32_t    Count;
  uint32_t    Position;
  float       Recent[NUM_FILTERDEEP];
} __PACKED_END TMiddleAvgFloatFilter;
//=============================================================================
// 本地常数
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 滤波器
static TMiddleAvgFloatFilter MAFltMod[NUM_ADC_CHANNELS];
static TMiddleAvgFloatFilter MAFltTemp[NUM_ADC_CHANNELS];
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 浮点数滤波
// 输入:
//   pFilter: 滤波器数据区
//    wValue: 新值
// 输出: 无
// 返回: 无
static float __MAF_FloatFilter ( TMiddleAvgFloatFilter *pFilter, float fValue )
{
  
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pFilter, GFC_EmptyPtr );
#endif
  
  // 滤波数据未存满？
  if( NUM_FILTERDEEP > pFilter->Count )
    {
    // 先积累
    pFilter->Count++;
    pFilter->Recent[pFilter->Position++] = fValue;
    }
  else
    {
    // 已经存满了
      
    // 当前位置卷回
    if( NUM_FILTERDEEP <= pFilter->Position )
      pFilter->Position = 0;
    // 当前值填入队列
    pFilter->Recent[pFilter->Position++] = fValue;
      
    // 求和、找最值
    float fSum = 0, fMax = -1E10, fMin = 1E10;
    for( int32_t uIdx = NUM_FILTERDEEP - 1; uIdx >= 0; uIdx-- )
      {
      fValue = pFilter->Recent[uIdx];
      if( fValue > fMax )
        fMax = fValue;
      if( fValue < fMin )
        fMin = fValue;
   
      // 求和 
      fSum += fValue;
      }
      
    // 排除最值，取平均
    fValue = (fSum - fMax - fMin) / (NUM_FILTERDEEP - 2);
    }
 
  return fValue; 
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化滤波器
// 输入:  无
// 输出:  无
// 返回:  无
void SFCMAF_Init ( void )
{

  memset( MAFltMod,  0, sizeof(MAFltMod)  );
  memset( MAFltTemp, 0, sizeof(MAFltTemp) );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 浮点数滤波
// 输入:
//     ftIdx: 滤波器序号
//   uChlIdx: 滤波器通道序号 0 ~ （NUM_ADC_CHANNELS - 1）
//    wValue: 新值
// 输出: 无
// 返回: 无
float SFCMAF_FloatFilter( TMAFilterIndex ftIdx, uint32_t uChlIdx, float fValue )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( NUM_ADC_CHANNELS <= uChlIdx, GFC_ErrParam );
#endif

  float fRes;
  switch( ftIdx )
    {
    case mftModulus:
      fRes = __MAF_FloatFilter( &MAFltMod[uChlIdx], fValue );
      break;
    
    case mftTemp:
      fRes = __MAF_FloatFilter( &MAFltTemp[uChlIdx], fValue );
      break;
    
#ifdef USE_DEV_ASSERT
    default:
      {
      DEV_FAULT( GFC_ErrParam );
      fRes = 0;
      break;
      }
#endif
    }

  return fRes;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
