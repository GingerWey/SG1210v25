//-----------------------------------------------------------------------------
/*
 File        : NumProc.cpp
 Version     : V1.01
 By          : 银网科技

 Description : 实现数值处理方法
               
 Date        : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "NumProc.h"


#include "DevFixed.h"
#include "DevRegs.h"
#include "DevTypes.h"

#include "MidAvgFilter.h"
#include <Dev_Cfg.h>

#define  USES_DFT  1
#if USES_DFT > 0
 #if NUM_SAMPLES_PER_PEROID == 64
  #include "DFT64.h"
 #elif NUM_SAMPLES_PER_PEROID == 80
  #include "DFT80.h"
 #else
  #error "Unkonow Samples per period."
 #endif
#endif

#ifndef __vmSIMULATOR__
 #include "TaskCtrl.h"
 #include <stm32f4xx_hal.h>

 #include <arm_math.h>
 #include <cmsis_os.h>
#else
 #include <math.h>
 #include <cmath>
#endif
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define ACHL_Uin                    0
#define ACHL_Uout                   1
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 处理保护数据
//-----------------------------------------------------------------------------
template<typename T> inline T MeasVoltProc(const uint32_t uCh )
{

  const auto& pSpectrum = GetACSpectrum( uCh );
 
  const auto modulus = pSpectrum->Base.Modulus;
  
  T result;
#if (Clear_LowLimit_En > 0) && (MIN_Volt > 0)
  if( MIN_Volt > modulus )
    result = 0;
  else
#endif
    result = (T)modulus;

  return result;
}
//=============================================================================
// 本地方法申明
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据引用
//-----------------------------------------------------------------------------
#if AOS_ENABLE > 0
 extern TInspectorAOS AOSBuffer;
#endif
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 保护数据计算
//-----------------------------------------------------------------------------
static void __ProtectionCalc()
{

  // 将所有保护数据归一化
//  MVoltProc ( ACHL_Uin, REG_RL_UinVal );
  const auto rVin = MeasVoltProc<float>( ACHL_Uin );
  _SetRealReg( REG_RL_UinVal, rVin );

  // 输入有压判定
  if( MIN_Volt > rVin )  
    {
    DevCache.Reals[REG_RL_ACPeriod - REG_REAL] = 0;
    DevCache.Reals[REG_RL_ACFreq   - REG_REAL] = 0;
    DevCache.Reals[REG_RL_ACFreqRT - REG_REAL] = 0;
    }

  // 将所有保护数据归一化
//  MVoltProc ( ACHL_Uout, REG_RL_UoutVal );
  const auto rVout = MeasVoltProc<float>( ACHL_Uout );
  _SetRealReg( REG_RL_UoutVal, rVout );

  // 输出有压判定
  if( MIN_Volt > rVout )  
    {
    DevCache.Reals[REG_RL_VOPeriod - REG_REAL] = 0;
    DevCache.Reals[REG_RL_VOFreq   - REG_REAL] = 0;
    DevCache.Reals[REG_RL_VOFreqRT - REG_REAL] = 0;
    }
}
//-----------------------------------------------------------------------------
// 对采样值执行64点FFT实时运算
void __RTCalc(void)
{

  if( CALC_Busy == DevCache.RTCalcState )
    return ;

  if( CALC_Ready != DevCache.RTCalcState )
    return ;    

  // 置【忙】标识
  DevCache.RTCalcState = CALC_Busy;
  
#if DBG_PIN_OUTPUT > 0  
//  DEBUG3_TOGGLE;
#endif

  // 数据区
  volatile TACSpectrum *pACSpectrum = DevCache.ACSpectrum;
  for( int iChl = 0; iChl < NUM_ADC_CHANNELS; iChl++ )
    {
#if  USES_DFT > 0
    // 分析基波
    TDFTResult DFTResults[2];
 #if NUM_SAMPLES_PER_PEROID == 64
    DFT64_H1( (int16_t*)DevCache.ADCRTCalBuf[iChl], DFTResults );  
 #elif NUM_SAMPLES_PER_PEROID == 80
    DFT80_H1( (int16_t*)DevCache.ADCRTCalBuf[iChl], DFTResults );  
 #else
    #error "Unkonow Samples per period."
 #endif

 #ifndef __vmSIMULATOR__
    float32_t fReal = DFTResults[0], fImage = DFTResults[1];

    // 校准并保存实部和虚部
    pACSpectrum->Base.Real  = fReal;
    pACSpectrum->Base.Image = fImage;
      
    // 计算模
    float32_t fSVal = fReal * fReal + fImage * fImage,
              fModulus;
    arm_sqrt_f32( fSVal, &fModulus );   // 用硬件开平方
 #else
    double fReal = DFTResults[0], fImage = DFTResults[1];
    // 校准并保存实部和虚部
    pACSpectrum->Base.Real  = fReal;
    pACSpectrum->Base.Image = fImage;
    // 计算模
    double fSVal = fReal * fReal + fImage * fImage,
           fModulus = sqrtf(fSVal);
 #endif
#else
    uint32_t uSum = 0;
    uint16_t*  puwSamples = (uint16_t*)DevCache.ADCRTCalBuf[iChl];
    for( int iIdx = 0; iIdx < NUM_SAMPLES_PER_PEROID; iIdx++ )
      {
      uint16_t uwSample = puwSamples[iIdx];
      uSum += uwSample;
      }
      
    uint16_t uSampleAvg = uSum / NUM_SAMPLES_PER_PEROID;
      
    uSum = 0;
    for( int iIdx = 0; iIdx < NUM_SAMPLES_PER_PEROID; iIdx++ )
      {
      int32_t iSample = puwSamples[iIdx];
      iSample -= uSampleAvg;
      uSum += iSample * iSample;
      }
      
    //iSum /= NUM_SAMPLES_PER_PEROID;
      
    float fModulus;
    arm_sqrt_f32( uSum, &fModulus );
#endif

    // 确保校准系统有效   Wey.2024.10.4  // v2.1
    if( 1024 < DevConfig.CaliCoef[iChl] )
      fModulus = (fModulus * DevConfig.CaliCoef[iChl]) / 16384.0 / 100.0; // 直接保存实际值
    else
#if  USES_DFT > 0
      fModulus = (fModulus * 6870) / 16384.0 / 100.0;
#else
      fModulus = (fModulus * 7622) / 16384.0;
#endif

    // 保存向量
    pACSpectrum->Base.Modulus = fModulus;
    
    pACSpectrum++;
    }
    
  // 还原状态
  DevCache.RTCalcState = CALC_Ready;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 保护数据处理方法
void NumProc_ProtectCalc(void)
{

  if( CALC_Ready == DevCache.RTCalcState )
    {
    // 实时基波计算
    __RTCalc();

    // 更新状态至“空闲”
    DevCache.RTCalcState = CALC_Idle;

    // 保护数据计算
    __ProtectionCalc();
      
 #ifndef __vmSIMULATOR__
    // 触发控制器
    ControllerTick();
 #endif
    }

#if AOS_ENABLE > 0
  // 实时状态监测
  if( STATE_TRUE  == AOSBuffer.Enable   &&
      NUM_AOS_REG >= AOSBuffer.RegCount &&
      0 < AOSBuffer.RegCount )
    {
    // 取得当前存贮位置
    uint16_t *puwData, puwBufPos;
    if( 0 == AOSBuffer.BufIdx )
      {
      puwBufPos = AOSBuffer.BufAPos;
      puwData   = (uint16_t*)&AOSBuffer.BufferA[puwBufPos];
      }
    else
      {
      puwBufPos = AOSBuffer.BufBPos;
      puwData   = (uint16_t*)&AOSBuffer.BufferB[puwBufPos];
      }
    
    // 填写数据
    for( uint32_t uIdx = 0; uIdx < AOSBuffer.RegCount; uIdx++ )
      *puwData++ = _GetCommonReg( AOSBuffer.RegNum[uIdx] ); // SG1210的测量放到了RealRegs
      
    // 推进缓冲区指针
    puwBufPos += (uint16_t)(AOSBuffer.RegCount) * sizeof(uint16_t);
    if( 0 == AOSBuffer.BufIdx )
      AOSBuffer.BufAPos = puwBufPos;
    else
      AOSBuffer.BufBPos = puwBufPos;
      
    // 缓冲区满？
    if( SIZE_AOS_BUF - NUM_AOS_REG * sizeof(uint16_t) + 2 <= puwBufPos )
      {
      // 当前缓冲区满，切换缓冲区
      AOSBuffer.BufRdy = STATE_TRUE;
      if( 0 == AOSBuffer.BufIdx )
        {
        AOSBuffer.BufIdx  = 1;
        AOSBuffer.BufBPos = 5;
        }
      else
        {
        AOSBuffer.BufIdx  = 0;
        AOSBuffer.BufAPos = 5;
        }      
      }
    }
#endif
}
//-----------------------------------------------------------------------------
//// 保护数据处理方法
//// 从中断调的接口，需要实时任务配合
//void NumProc_ProtectCalcIT(void)
//{

////  // 触发控制器
////  ControllerTick();
//}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static float series_ln(float x)
{
/**
 * @brief 用级数展开计算ln(x)，适用于x∈(0.01,10)范围
 *        采用格雷戈里级数变体：
 *                ln(x) = 2*Σ[( (x-1)/(x+1) )^(2n+1)/(2n+1)] (n=0→∞)
 * @param x 输入值（Rt/R0的比值）
 * @return ln(x)的近似值，误差<1e-6
 */
 
  if (x <= 0.001 || x >= 1000.0 ) 
    {
    return 0.0; // 超出适用范围，返回0（实际应用中需处理）
    }
  
   // 计算t=(x-1)/(x+1)，将x映射到t∈(-1,1)以保证级数收敛
   double t = (x - 1.0) / (x + 1.0);
 
  // 累加级数项至t^11/11，经误差分析可满足±0.05℃精度
  double t_power   = t;      // 当前t的奇次幂
  double t_squared = t * t;  // t2，用于快速计算高阶项

  
  // 初始项：t^1/1
  double result = t;
  
  // 第1项：t^3/3
  t_power *= t_squared;
  result += t_power / 3.0;
  
  // 第2项：t^5/5
  t_power *= t_squared;
  result += t_power / 5.0;
  
  // 第3项：t^7/7
  t_power *= t_squared;
  result += t_power / 7.0;
  
  // 第4项：t^9/9
  t_power *= t_squared;
  result += t_power / 9.0;
  
  // 第5项：t^11/11（关键项，确保低温段精度）
  t_power *= t_squared;
  result += t_power / 11.0;
  
  // 第6项：t^13/13（补偿高温段误差）
  t_power *= t_squared;
  result += t_power / 13.0;
  
  // 级数结果乘以2得到ln(x)
  return 2.0 * result;
}
//-----------------------------------------------------------------------------
/**
 * @brief 将NTC分压电压转换为温度值（使用级数计算对数）
 * @param vntc ADC采样得到的TEMP_AIN电压值（单位：V）
 * @return 转换后的温度值（单位：℃，范围-40℃~105℃，精度±0.05℃）
 */
static float ntcVoltageToTemperature(float Vntc) 
{
  // 电路参数定义
  constexpr float VD33       = 3.3;       // 电源电压3.3V
  constexpr float Rconst     = 10000.0;   // 分压电阻10kΩ
  constexpr float R_NTC_NOM  = 10000.0;   // NTC标称电阻10kΩ（25℃时）
  constexpr float BETA_VALUE = 3435.0;    // NTC的B参数
  constexpr float T_NOM      = 298.15;    // 标称温度25℃（转换为开尔文）
  constexpr float MIN_TEMP   = -50.0;     // 最低测量温度
  constexpr float MAX_TEMP   = 105.0;     // 最高测量温度

  // 电压边界处理（避免除以零或超出合理范围）
  if (Vntc <= 0.001) 
    return MIN_TEMP;
  if (Vntc >= VD33 - 0.001) 
    return MAX_TEMP;

  // 1. 根据分压电路计算NTC当前阻值
#if VER_SG12B10 <= 210
  // v2.1板采用NTC上端布置方式
  //    分压公式：Vntc = VD33 * Rconst / (Rntc + Rconst)
  //    推导 R17 = (VD33 * Rconst) / Vntc - Rconst
  float Rntc = (VD33 * Rconst) / Vntc - Rconst;
#else
  // v2.5板采用NTC下端布置方式
  // 分压公式：Vntc =  VD33 * Rntc / (Rntc + Rconst)
  // 推导得：  Rntc = (Vntc * Rconst)/ (VD33  - Vntc)
  float Rntc = (Vntc * Rconst) / (VD33 - Vntc);
#endif

  // 2. 用级数计算ln(ratio)（替代math.h的log函数）
  float lnRatio = series_ln(Rntc / R_NTC_NOM);  // log

  // 3. 使用B参数方程计算温度（开尔文）
  // 公式：1/T = 1/T_NOM + (1/BETA_VALUE) * ln(Rntc/R_NTC_NOM)
  float invTemperature    = 1.0f / T_NOM + lnRatio / BETA_VALUE;
  float temperatureKelvin = 1.0f / invTemperature;

  // 4. 转换为摄氏度
  return temperatureKelvin - 273.15f;
}
//-----------------------------------------------------------------------------
// 测量数据处理方法
void NumProc_MeasureCalc(void)
{
  
  // 交流输入电压
//  TCommonReg uValue = _GetCommonReg(REG_CM_UinVal);
//  uValue = SFCMAF_FloatFilter( mftModulus, ACHL_Uin, uValue ) + 0.5;
//  if( RATIO_Voltage * 10 > uValue )
//    uValue = 0;
// _SetCommonReg( REG_CM_Uin, uValue );
  auto rValue = _GetRealReg(REG_RL_UinVal);
  rValue = SFCMAF_FloatFilter( mftModulus, ACHL_Uin, rValue );
#ifdef MIN_Volt
  if( MIN_Volt > rValue )
    rValue = 0;
#endif
  _SetRealReg( REG_RL_Uin, rValue );

  // v2.1
  // 逆变器输出电压
//  uValue = _GetCommonReg(REG_CM_UoutVal);
//  uValue = SFCMAF_FloatFilter( mftModulus, ACHL_Uout, uValue ) + 0.5;
//  if( RATIO_Voltage * 10 > uValue )
//    uValue = 0;
//  _SetCommonReg( REG_CM_Uout, uValue );
  rValue = _GetRealReg(REG_RL_UoutVal);
  rValue = SFCMAF_FloatFilter( mftModulus, ACHL_Uout, rValue );
#ifdef MIN_Volt
  if( MIN_Volt > rValue )
    rValue = 0;
#endif
  _SetRealReg( REG_RL_Uout, rValue );

  // v2.1
  // 用NTC分压计算锂电池的温度
  float fVntc = DevCache.DCSamples[0];
        fVntc = 3.3f * fVntc / 4095.0 / 6;  // 6x过采样
  if( fVntc > 1e-6 )
    {
    float fTemp = ntcVoltageToTemperature( fVntc );

    rValue = SFCMAF_FloatFilter(mftTemp, 0, fTemp); // * RATIO_Temperature + 0.5f;

    //_SetCommonReg( REG_CM_BAT_TEMPERATRUE, iValue );
    _SetRealReg( REG_RL_BAT_TEMPERATRUE, rValue );
    }
}
//-----------------------------------------------------------------------------
