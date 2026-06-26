//-----------------------------------------------------------------------------
/*
 File        : SamplesProc.c
 Version     : V1.10
 By          : 银网科技

 Description :定义交流采样点处理过程
              1.计算64点采样的方法
                输入在DevCache.ADCSamples
                输出在FREG_A0xxx寄存器
              2.逐点校正
              3.计算有效值和幅角
        
 采用ST的DSP运算库
        
 Date       : 2023.12.05
 
   1. 移植到GPT30
   2. 采用64点采样
*/
//-----------------------------------------------------------------------------
#include "SamplesProc.h"

#include "NumProc.h"

#include "DevRegs.h"
#include "DevDebug.h"

#include "ADCMgr.h"

#if WAVELOGGER_EN > 0 && NUM_WAVERECORDER > 0 
  #include "WaveLogger.h"
#endif

#ifndef __SIMULATOR__
  #include "dio.h"
  #include "gpio.h"  // debug only
  #include "ADCMgr.h"

 #if SYNC_LOGIC_EN == 0
  extern TIM_HandleTypeDef htim7;    // RT Calc
 #endif
#endif

// 是否使用 软件模拟AD采样
// USES_SIMADC > 0时, 可由软件产生正弦周期的采样数据, 用于测试计算过程
#define  USES_SIMADC     0
#if USES_SIMADC > 0
  #include "SimADC.h"
#endif  

#include <string.h>
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
//=============================================================================
// 全局数据引用
//-----------------------------------------------------------------------------
// 
#ifndef __SIMULATOR__
// #if SYNC_LOGIC_EN == 0
//  extern TIM_HandleTypeDef htim7;
// #endif
#endif
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define SIZE_DSPFFT      NUM_SAMPLES_PER_PEROID
#define SIZE_SAMPLES     (SIZE_DSPFFT)
//-----------------------------------------------------------------------------
// 调试开关
#ifdef __DEBUG
  #define DBG_PIN_OUTPUT   1       // 通过Debug管脚输出信号
  #define DBG_TIME_MEASURE 1       // 测量代码段运行的时间
#else
  #define DBG_PIN_OUTPUT   0       // 通过Debug管脚输出信号
  #define DBG_TIME_MEASURE 0       // 测量代码段运行的时间
#endif

//// 调试用采样点
//uint16_t  FuDbgSamples[SIZE_RTCALC_BUFFER];
//uint32_t  FMeas, FError = 0;
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 128阶求模开方查找表

//=============================================================================
// 本地常数
//-----------------------------------------------------------------------------
#if SAMPLE_FIR_EN > 0
  // FIR_0.1/0.35
  // 采样频率 4000Hz 通过频率400Hz(8次) 截止频率1400Hz(28次）
  constexpr float fFirTag12[] = 
    {
     0.00292332118151176f, 
    -0.00869499159467325f, 
    -0.04188992536052520f, 
    -0.01165470144289710f, 
     0.17140649929895400f, 
     0.38353368161275800f, 
     0.38353368161275800f, 
     0.17140649929895400f, 
    -0.01165470144289710f, 
    -0.04188992536052520f, 
    -0.00869499159467325f, 
     0.00292332118151176f 
    };
  #define FIR_TAGs    fFirTag12
  #define DEEP_FIR    (sizeof(FIR_TAGs) / sizeof(FIR_TAGs[0]))
#endif

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
//
//#define  NUM_SampleDebug       1024
//uint16_t DCSampleDbg[NUM_SampleDebug] = {0};
//uint16_t DCSampleDbgPtr = 0;
//=============================================================================
// 全局方法引用
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#if SAMPLE_FIR_EN > 0
// FIR滤波
uint32_t SAMCAL_FIR(uint32_t uChl, uint32_t uValue)
{
  
  DevCache.ADCFIRBuffer[uChl][DevCache.FIR_BufPtr] = uValue;
  
  if( SIZE_RT_FIR > DevCache.FIR_BufCntr )
    return uValue;
  
  uint32_t   uPtr  = DevCache.FIR_BufPtr + 1;
  float      fSum  = 0;
  const auto pBuff = DevCache.ADCFIRBuffer[uChl];
  for( uint32_t uIdx = 0; uIdx < SIZE_RT_FIR; uIdx++ )
    {
    if( SIZE_RT_FIR <= uPtr )
      uPtr = 0;
      
    fSum += (float)pBuff[uPtr] * FIR_TAGs[uIdx];
    uPtr++;
    }
    
  return (uint32_t)(fSum + 0.5f);
}
#endif
//-----------------------------------------------------------------------------
// 填写ADC缓冲区
void SAMCAL_FillADCCache( const TADCRanksDef* pADCRanks )
{
  
  // 防止指针越界
  if( SIZE_RTCALC_BUFFER <= DevCache.SampBufPtr )
    {
    DevCache.SampBufCnt = 0;
    DevCache.SampBufPtr = 0;
    }

#if SPECTRUM_EN > 0
  if( SIZE_ANALYSER_BUFFER < DevCache.AnalyserPtr )
    {
    DevCache.AnalyserPtr = 0;
    DevCache.AnalyserState = CALC_Idle;
    }
#endif    
  // ... 从ADC中取数据
  for( uint32_t uChl = 0; uChl < NUM_ADC_CHANNELS; uChl++ )
    {
    uint32_t uSample = DevCache.ADCCrsSection[uChl];   

#if SIZE_OSCI_BUFFER >  0
    // 实时波形
    if( DevCache.OSCIBufferPtr < SIZE_OSCI_BUFFER )
      DevCache.OSCIBuffer[uChl][DevCache.OSCIBufferPtr] = uSample;
#endif

#if SAMPLE_FIR_EN > 0
    // 执行FIR
    uSample = SAMCAL_FIR( uChl, uSample );
#endif

    // 保存到实时计算数据区
    // 填写采样缓冲区
    DevCache.ADCSamples[uChl][DevCache.SampBufPtr] = uSample;
      
    // 在实时计算完成后，准备下一次实时计算需要的数据
    if( CALC_Idle == DevCache.RTCalcState && 
        SIZE_RTCALC_SAMPLES <= DevCache.SampBufCnt )
      {
      if( SIZE_RTCALC_SAMPLES <= DevCache.SampBufPtr + 1 )
        memcpy( (void*)(DevCache.ADCRTCalBuf[uChl]),
                (void*)(DevCache.ADCSamples[uChl] + (DevCache.SampBufPtr + 1) - SIZE_RTCALC_SAMPLES),
                SIZE_RTCALC_SAMPLES * sizeof(TACSample) );
      else
        {
        // 取卷回之间的采样点
        memcpy( (void*)(DevCache.ADCRTCalBuf[uChl]),
                (void*)(DevCache.ADCSamples[uChl] + 
                         SIZE_RTCALC_BUFFER  - 
                        (SIZE_RTCALC_SAMPLES - (DevCache.SampBufPtr + 1))),
                (SIZE_RTCALC_SAMPLES - (DevCache.SampBufPtr + 1)) * sizeof(TACSample) );
        // 取卷回之后的采样点
        memcpy( (void*)(DevCache.ADCRTCalBuf[uChl] + 
                        SIZE_RTCALC_SAMPLES - (DevCache.SampBufPtr + 1)),
                (void*)(DevCache.ADCSamples[uChl]),
                (DevCache.SampBufPtr + 1) * sizeof(TACSample) );
        }
      }  
    }
    
#if SIZE_OSCI_BUFFER > 0
    // 实时波形
    if( DevCache.OSCIBufferPtr < SIZE_OSCI_BUFFER )
      DevCache.OSCIBufferPtr++;
    else if( GetHWFault( RHF_Freq_MIS ) )
      DevCache.OSCIBufferPtr = 0;
#endif

  // 更新采样缓冲区指针
  // 实时计算空闲，并且数据区准备完毕，可以进行实时计算
  if( CALC_Idle == DevCache.RTCalcState && 
      SIZE_RTCALC_BUFFER <= DevCache.SampBufCnt )
    {
    // 数据准备就绪
    DevCache.RTCalcState = CALC_Ready;

#ifndef __SIMULATOR__
  #if SYNC_LOGIC_EN > 0
//   if( RDM_NORMAL == GetDevMode || 
//       RDM_CALIB  == GetDevMode || 
//       RDM_TEST   == GetDevMode )
      {
      // 保护数据计算
      NumProc_ProtectCalc();
      }
      
    // 计算工作完成 
    DevCache.RTCalcState = CALC_Idle;

    // 置ADC任务有效
    SetRSTSrc( RRS_ADCTASK );
  #else
    // 启动定时器，
    HAL_TIM_Base_Start_IT( &htim7 );
  #endif  // SYNC_LOGIC_EN
#endif
    }

  DevCache.SampBufPtr++;
  if( SIZE_RTCALC_BUFFER <= DevCache.SampBufPtr )
    {
    DevCache.SampBufPtr = 0;
        
    // 在指针卷回后，一次性更新采样数据区计数
    if( SIZE_RTCALC_BUFFER > DevCache.SampBufCnt )
      {
      DevCache.SampBufCnt = SIZE_RTCALC_BUFFER;
      }
    }

  // 更新采样计数器指针
  DevCache.SamplesPtr++;
  if( MAX_SAMPLE_PTR <= DevCache.SamplesPtr )
    DevCache.SamplesPtr = 0;

#if SAMPLE_FIR_EN > 0
  // 更新FIR滤波器指针
  DevCache.FIR_BufPtr++;
  if( SIZE_RT_FIR <= DevCache.FIR_BufPtr )
    DevCache.FIR_BufPtr = 0;
  
  if( SIZE_RT_FIR > DevCache.FIR_BufCntr )
    DevCache.FIR_BufCntr++;
#endif
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 实始化
void SAMCAL_Init(void)
{

  DevCache.SamplesPtr = 0;
  DevCache.SampBufPtr = 0;
  DevCache.SampBufCnt = 0;

  DevCache.RTCalcState   = CALC_Idle;
  DevCache.AnalyserState = CALC_Idle;
  DevCache.MeasureState  = CALC_Idle;

  memset( (void*)DevCache.ADCCrsSection, 0, sizeof(DevCache.ADCCrsSection) );

#if USES_SIMADC > 0
  SimADC_Init();
#endif  
}
//-----------------------------------------------------------------------------
// 读取ADC结果
/**
  * @brief  Regular conversion complete callback in non blocking mode 
  * @param  hadc: pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{

  memset( (void*)DevCache.ADCCrsSection, 0, sizeof(DevCache.ADCCrsSection) );

  // 累计缓冲区
  uint32_t uSampSum[NUM_ADC_CHANNELS];
  memset( (void*)uSampSum, 0, sizeof(uSampSum) );
  uint32_t uDCSampleSum = 0;

//  auto pADCSamples = FuADCSamples;
//  const uint8_t* pChannels = (const uint8_t*)FucADCChannels;
  for( uint32_t uIdx = 0; uIdx < ADC_NbrOfConversion; uIdx++ )
    {
    // 1st request & 2nd & 3rd request
    for( uint32_t uAdc = 0; uAdc < 3; uAdc++ )
      {
      uint32_t uSample  = (uint32_t)FuADCSamples[uIdx][uAdc];
      uint32_t uChannel = FucADCChannels[uIdx][uAdc];
      switch( uChannel )
        {
        case ADCH_VACin:
          uSampSum[0]  += uSample;
          break;
        case ADCH_VACout:
          uSampSum[1]  += uSample;
          break;
        case ADCH_Temp:
          uDCSampleSum += uSample;
          break;
        }
      }
    }

  DevCache.ADCCrsSection[0] = (uSampSum[0] + 3) / 7;
  DevCache.ADCCrsSection[1] = (uSampSum[1] + 3) / 7;
    
  DevCache.DCSamples[0]     = uDCSampleSum;  // 保存6倍过采样结果

//  if( NUM_SampleDebug <= DCSampleDbgPtr )
//    DCSampleDbgPtr = 0;
//  DCSampleDbg[DCSampleDbgPtr++] = DevCache.DCSamples[0];

  // 填写采样Buffer    
  SAMCAL_FillADCCache( nullptr );
  
#if WAVELOGGER_EN > 0 && NUM_WAVERECORDER > 0
 #if NUM_SAMPLES_PER_PEROID == NUM_SAMPLOG_PER_PEROID * 2
   if( 1 == (DevCache.SampBufPtr & 1) )
 #elif NUM_SAMPLES_PER_PEROID == NUM_SAMPLOG_PER_PEROID * 4
   if( 1 == (DevCache.SampBufPtr & 3) )
 #elif NUM_SAMPLES_PER_PEROID == NUM_SAMPLOG_PER_PEROID * 8
   if( 1 == (DevCache.SampBufPtr & 7) )
 #elif NUM_SAMPLES_PER_PEROID != NUM_SAMPLOG_PER_PEROID
   #error Incorrect NUM_SAMPLOG_PER_PEROID!
 #endif  
     // 录波
     WAVELOG_Record();
#endif

//  // 开入扫描
//  DIO_SignalScan();
}  
//-----------------------------------------------------------------------------
