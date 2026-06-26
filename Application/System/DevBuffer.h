//-----------------------------------------------------------------------------
/*
 File        : DevBuffer.h
 Version     : V1.10
 By          : 银网科技

 Description :定义应用缓存区和方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef DEV_BUFFER_H
#define DEV_BUFFER_H

#include "Dev_Cfg.h"
#include "DevTypes.h"

#include "MidAvgFilter.h"
#include "DevBuffer.h"
#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
#define SIZE_OSCI_BUFFER       0
  
#define SIZE_DIM_CACHE         8
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------
// 缓冲区状态
typedef enum __TBufferState
{
  CALC_Idle  = 0,               // 空闲
  CALC_Ready = 0x55,            // 采样完成
  CALC_Busy  = 0xAA             // 正在计算
} TBufferState;
//-----------------------------------------------------------------------------
// 应用缓冲区结构
typedef struct tagDevCache
{
  
  // 带校验的状态寄存器数据区
  uint32_t StateCRC;
  volatile TStateReg    State[ (NUM_STATE_REGS + 3) & ~3ul];
  // 通用寄存器
  volatile TCommonReg   Common[NUM_COMMON_REGS];
  // 浮点寄存器
  volatile TRealReg     Reals[NUM_REAL_REGS];
  
  // 边界0
  volatile uint32_t   Border0;
  
  // 系统状态
  volatile TDevStateReg  DeviceState[NUM_DEVICE_STATES];
  
  // IO状态
  volatile TStateReg     IOState[NUM_IO_STATES];
  
  // 边界0
  volatile uint32_t   Border1;
  
  // 各种状态
  volatile uint32_t     SamplesPtr,                 // 采样指针
                        SampBufPtr,                 // 当前采样点存贮的位置
                        SampBufCnt;                 // 

  volatile TBufferState RTCalcState,                // 实时计算器的状态
                        AnalyserState,              // 频谱分析器的状态
                        MeasureState;               // 测量计算状态

#if SAMPLE_FIR_EN > 0
  volatile uint32_t     FIR_BufCntr,                // FIR滤波空间数据量
                        FIR_BufPtr;                 // FIR数据指针
#endif
  
#if SIZE_OSCI_BUFFER >  0
  // 示波器缓冲区
  volatile uint32_t     OSCIBufferPtr;              // 实时计算器的状态
#endif

#if SPECTRUM_EN > 0
  volatile uint16_t AnalyserPtr;                  // 频谱分析器数据区指针
#endif
  
  // 采样截面
  volatile TACSample    ADCCrsSection[NUM_ADC_CHANNELS];

  // 交流采样存贮区
  volatile TACSample    ADCSamples[NUM_ADC_CHANNELS][SIZE_RTCALC_BUFFER];
  // 用于实时计算的存贮区
  // 按DFT的要求，只放实部
  volatile TACSample    ADCRTCalBuf[NUM_ADC_CHANNELS][SIZE_RTCALC_SAMPLES];

#if SIZE_OSCI_BUFFER >  0
  // 示波器缓冲
  volatile uint32_t     OSCIBuffer[NUM_ADC_CHANNELS][SIZE_OSCI_BUFFER];
#endif

#if SAMPLE_FIR_EN > 0
  // FIR 
  volatile TACSample    ADCFIRBuffer[NUM_ADC_CHANNELS][SIZE_RT_FIR]; 
#endif

#if SPECTRUM_EN > 0
  // 用于频谱分析的存贮区
  // 按DFT的要求，只存实部
  TACSample   ADCSpectBuf[NUM_ADC_CHANNELS][SIZE_ANALYSER_BUFFER];
#endif
  // 频谱分析结果
  volatile TACSpectrum  ACSpectrum[NUM_ADC_CHANNELS];

#if NUM_DCS_CHANNELS > 0
  // 直流采样存贮区
  volatile TDCSample    DCSamples[NUM_DCS_CHANNELS];
#endif

  // 实时时间
  TDateTimeType  DateTime;                 // 用于时间编辑的缓冲区
  
  char           DIMBuffer[8];             // 用于量纲字符存贮的空间
} TDevCache, *TDevCachePtr;
//=============================================================================
// 全局数据申明
//-----------------------------------------------------------------------------
// 系统缓冲区
extern TDevCache DevCache;
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void DevBuf_Init(void);
//-----------------------------------------------------------------------------
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
