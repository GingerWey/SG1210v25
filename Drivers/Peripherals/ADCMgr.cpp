//-----------------------------------------------------------------------------
/*
 File        : ADCMgr.c
 Version     : V1.10
 By          : 银网科技

 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description :管理ADC过程
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "ADCMgr.h"

#include "adc.h"
#include "tim.h"
#include "gpio.h"

#include "INA226.h"
#include "SD3077.h"

#include "DevRegs.h"
#include "DevEvtMgr.h"

#include "SamplesProc.h"
#include "MidAvgFilter.h"

#include "DevDebug.h"

#include <string.h>
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// 周期测量滤波器深度
#define  DEEP_PERIOD_FILTER     6
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
typedef struct tagFreqCapturer
{
  uint8_t  ucResRegistersOffset;
  uint8_t  ucCaptureIndex;
  uint8_t  ucFilterCntr, ucFilterPtr;
  
  uint32_t uCapValue1, uCapValue2;
  uint32_t uFilterCache[DEEP_PERIOD_FILTER];
  
  uint32_t uTimerClkFreq;
} TFreqCapturer;
//-----------------------------------------------------------------------------
// 输入电压频率测量
TFreqCapturer  Freq1Capturer;

// 输出电压频率测量
TFreqCapturer  Freq2Capturer;
//=============================================================================
// 局部方法
//-----------------------------------------------------------------------------

//=============================================================================
// 局部方法实现
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
// DMA的采样结果区
// 在三重 ADC 模式下，发出第一个请求时传输 ADC1 的数据，发出第二个请求时传输    
// ADC2 的数据，发出第三个请求时传输 ADC3 的数据；重复此序列。因此 DMA 首先     
// 传输 ADC1 的数据，随后传输 ADC2 的数据，再传输 ADC3 的数据，依次类推。
// DMA 模式 1 用于三重规则同时模式。
// 示例：
//   三重规则同时模式：生成 3 个连续的 DMA 请求（每个请求对应一个转换数据项）
//       第 1 个请求：ADC_CDR[31:0] = ADC1_DR[15:0]
//       第 2 个请求：ADC_CDR[31:0] = ADC2_DR[15:0]
//       第 3 个请求：ADC_CDR[31:0] = ADC3_DR[15:0]
//       第 4 个请求：ADC_CDR[31:0] = ADC1_DR[15:0]
volatile uint16_t FuADCSamples[ADC_NbrOfConversion][3];
//-----------------------------------------------------------------------------
// 采样通道定义
const uint8_t FucADCChannels[ADC_NbrOfConversion][3] =
{
   { ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 0
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 1
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 2
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 3
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 4
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 5
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 6
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 7
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 8
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 9
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 10
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 11
  ,{ ADCH_VACin,    ADCH_VACin,    ADCH_VACin  }      // 12
  ,{ ADCH_VACout,   ADCH_VACout,   ADCH_VACout }      // 13
  ,{ ADCH_Temp,     ADCH_Temp,     ADCH_Temp   }      // 14
  ,{ ADCH_Temp,     ADCH_Temp,     ADCH_Temp   }      // 15
}; 
//=============================================================================
// 全局方法实现
//-----------------------------------------------------------------------------
// 初始化
void ADCMGR_Init(void)
{

  // 初始化ADC硬件
  MX_ADC_Init();
  
  // 频率捕捉器
  memset( &Freq1Capturer, 0, sizeof(Freq1Capturer) );
  memset( &Freq2Capturer, 0, sizeof(Freq2Capturer) );
  // TIM2在APB1上，APB1上的定时器是其频率的2倍
  Freq1Capturer.uTimerClkFreq = 2 * HAL_RCC_GetPCLK1Freq();
  Freq2Capturer.uTimerClkFreq = 2 * HAL_RCC_GetPCLK1Freq();
  Freq1Capturer.ucResRegistersOffset = 0;
  Freq2Capturer.ucResRegistersOffset = REG_RL_VOPeriod - REG_RL_ACPeriod;
  
  // 初始化底板电池充电测量
  uint32_t uINA226Id = INA226_Init(0);
  
  // 初始化电池充电测量
  uint32_t uConfig;
  int iRes = INA226_ReadConfig(0, &uConfig);
  if( 0x54492260 != uINA226Id || 0 != iRes )
    {
    SetHWFault( RHF_ExADC1_ERR );

    EVTMGR_AppendEvent( REG_EH_EXADC1_FAULT, EVENT_TRUE );
    }
  else
    ClrHWFault( RHF_ExADC1_ERR );
  
  uINA226Id = INA226_Init(1);
  
  // 初始化电池放电测量
  uINA226Id = INA226_Init(1);
  iRes = INA226_ReadConfig(1, &uConfig);
  if( 0x54492260 != uINA226Id || 0 != iRes )
    {
    SetHWFault( RHF_ExADC2_ERR );

    EVTMGR_AppendEvent( REG_EH_EXADC2_FAULT, EVENT_TRUE );
    }
  else
    ClrHWFault( RHF_ExADC2_ERR );
  
  // 读RTC温度及电池电压
  ADCMDR_ReadRTCMeasure();
}
//-----------------------------------------------------------------------------
// 启动采样
void ADCMGR_Start(void)
{

  // 采样计算处理器初始化
  SAMCAL_Init();
  
  // 滤波器
  SFCMAF_Init();

  HAL_TIM_Base_Start_IT(&htim2);    // 频率捕捉

  HAL_TIM_Base_Start_IT(&htim3);    // 采样中断
}
//-----------------------------------------------------------------------------
// 由分频中断驱动的采样开始
void ADC_Startup(void)
{

  HAL_ADC_Start(&hadc3);
  HAL_ADC_Start(&hadc2);
  if(HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*)FuADCSamples, 48) != HAL_OK)
    {
    SetHWFault( RHF_ADC_ERR );
    }
}
//-----------------------------------------------------------------------------
// 停止采样
void ADCMGR_Stop(void)
{
}
//-----------------------------------------------------------------------------
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{

  if( RESET != __HAL_ADC_GET_FLAG( hadc, ADC_SR_OVR ) )
    __HAL_ADC_CLEAR_FLAG( hadc, ADC_SR_OVR );

  HAL_ADC_Stop_DMA( &hadc1 );
  HAL_ADC_Stop_DMA( &hadc2 );
  HAL_ADC_Stop_DMA( &hadc3 );
  
  hadc->State = HAL_ADC_STATE_READY;

  ADC_Startup();
}
//=============================================================================
//-----------------------------------------------------------------------------
// 计算频率捕获周期值
static uint32_t _Calc_CapturePeriod(TFreqCapturer *pCapturer)
{

  // Capture computation
  uint32_t uDiffCapture;
  if ( pCapturer->uCapValue2 > pCapturer->uCapValue1 )
    {
    uDiffCapture = pCapturer->uCapValue2 - pCapturer->uCapValue1;
    }
  else if ( pCapturer->uCapValue2 < pCapturer->uCapValue1 )
    {
    // 0xFFFFFFFF is max TIM2_CCRx value
    uDiffCapture = ( ( 0xFFFFFFFF - pCapturer->uCapValue1 ) + 
                     pCapturer->uCapValue2 ) + 1;
    }
  else
    {
    // If capture values are equal,
    // we have reached the limit of frequency measures
    uDiffCapture = 0;
    }

  // 计算周期
  uint32_t uPeriod = 0;
  if ( uDiffCapture > 1400000 && uDiffCapture < 2100000 )
    {
    // 中值滤波
    pCapturer->uFilterCache[pCapturer->ucFilterPtr++] = uDiffCapture;
    if ( pCapturer->ucFilterPtr >= DEEP_PERIOD_FILTER )
      pCapturer->ucFilterPtr = 0;

    if ( pCapturer->ucFilterCntr < DEEP_PERIOD_FILTER )
      {
      pCapturer->ucFilterCntr++;
      }
    else
      {
      uint32_t uMax = 0, uMin = -1u, uSum = 0;
      for ( uint32_t uIdx = 0; uIdx < DEEP_PERIOD_FILTER; uIdx++ )
        {
        if ( pCapturer->uFilterCache[uIdx] > uMax )
          uMax = pCapturer->uFilterCache[uIdx];
        else if( pCapturer->uFilterCache[uIdx] < uMin )
          uMin = pCapturer->uFilterCache[uIdx];

        uSum += pCapturer->uFilterCache[uIdx];
        }

      // 处理成周期和频率
#if DEEP_PERIOD_FILTER == 10
      uPeriod  = ( uSum - uMax - uMin ) >> 3;
#elif DEEP_PERIOD_FILTER == 6
      uPeriod  = ( uSum - uMax - uMin ) >> 2;
#elif DEEP_PERIOD_FILTER > 2
      uPeriod  = ( uSum - uMax - uMin ) / ( DEEP_PERIOD_FILTER - 2 );
#else
  #error "DEEP_PERIOD_FILTER must greater than 2"
#endif

      // APB1上的定时器是其频率的2倍
      uint32_t uTimFreq = pCapturer->uTimerClkFreq,
#ifdef RATIO_Freq
               uFreq    = (uint64_t)uTimFreq * RATIO_Freq / uPeriod;
      // 频率在有效范围，才可接收
      if ( 42 * RATIO_Freq < uFreq && 58 * RATIO_Freq > uFreq )
#else
               uFreq    = (uint64_t)uTimFreq * 100 / uPeriod;
      // 频率在有效范围，才可接收
      if ( 4200 < uFreq && 5800 > uFreq )
#endif
        {
#ifdef REG_ACPeriod
        uint32_t uRegNUm = pCapturer->ucResRegistersOffset + 
                           (REG_ACPeriod - REG_COMMON);
        DevCache.Common[uRegNUm++] = uPeriod / (uTimFreq / 1000000); // 以us为单位
        DevCache.Common[uRegNUm++] = uFreq;
        DevCache.Common[uRegNUm]   = (uint64_t)uTimFreq * RATIO_Freq / uDiffCapture;
#else
        const auto fFreq   = (float)uTimFreq / uPeriod;
        auto uRegNum = pCapturer->ucResRegistersOffset + 
                           (REG_RL_ACPeriod - REG_REAL);
        DevCache.Reals[uRegNum++] = uPeriod / (uTimFreq / 1000000); // 以us为单位
        DevCache.Reals[uRegNum++] = fFreq;
        DevCache.Reals[uRegNum]   = (float)uTimFreq / uDiffCapture;
#endif
        }
      else
        uPeriod = 0;
      }
    }
  else
    {
    }
    
  return uPeriod;
}
//-----------------------------------------------------------------------------
// 频率捕捉回调
void HAL_TIM_IC_CaptureCallback ( TIM_HandleTypeDef *htim )
{

  // Freq1 = B.10 = TIM2CH3
  if ( htim->Instance == TIM2 &&
       htim->Channel  == HAL_TIM_ACTIVE_CHANNEL_3 )
    {
    if ( Freq1Capturer.ucCaptureIndex == 0 )
      {
      // Get the 1st Input Capture value
      Freq1Capturer.uCapValue1 = TIM2->CCR3;
      Freq1Capturer.ucCaptureIndex = 1;
      }
    else 
      {
      if ( Freq1Capturer.ucCaptureIndex == 1 )
        {
        // Get the 2nd Input Capture value
        Freq1Capturer.uCapValue2 = TIM2->CCR3;
        
        // Capture computation
        uint32_t uPeriod = _Calc_CapturePeriod( &Freq1Capturer );
        if( 0 < uPeriod )
          {
          // 跟踪电网周期，设置采样周期
          // 按NUM_SAMPLES_PER_PEROID分频
          // TIM3和TIM2同在APB1，uPeriod不需要转换
          TIM3->ARR = ( uPeriod + ( NUM_SAMPLES_PER_PEROID / 2 ) ) /
                      NUM_SAMPLES_PER_PEROID;
            
          // 清错误信息
          ClrHWFault ( RHF_Freq_MIS );
          }
        }

      Freq1Capturer.ucCaptureIndex = 0;
      }
    }
  else   
  // Freq2 = B.11 = TIM2CH4
  if ( htim->Instance == TIM2 &&
       htim->Channel  == HAL_TIM_ACTIVE_CHANNEL_4 )
    {
    if ( Freq2Capturer.ucCaptureIndex == 0 )
      {
      // Get the 1st Input Capture value
      Freq2Capturer.uCapValue1 = TIM2->CCR4;
      Freq2Capturer.ucCaptureIndex = 1;
      }
    else 
      {
      if ( Freq2Capturer.ucCaptureIndex == 1 )
        {
        // Get the 2nd Input Capture value
        Freq2Capturer.uCapValue2 = TIM2->CCR4;
        
        // Capture computation
        uint32_t uPeriod = _Calc_CapturePeriod( &Freq2Capturer );
        if( 0 < uPeriod )
          {
          // 输入电压频率不存在时，同步到输出电压频率上
#ifdef REG_ACPeriod
          if( 0 == DevCache.Common[REG_ACPeriod - REG_COMMON] )
#else
          if( 0 == DevCache.Reals[REG_RL_ACPeriod - REG_REAL] )
#endif
            {
            // 按NUM_SAMPLES_PER_PEROID分频
            // TIM3和TIM2同在APB1，uPeriod不需要转换
            TIM3->ARR = ( uPeriod + ( NUM_SAMPLES_PER_PEROID / 2 ) ) /
                        NUM_SAMPLES_PER_PEROID;
            }

          // 清错误信息
          ClrHWFault ( RHF_Freq_MIS );
          }
        }

      Freq2Capturer.ucCaptureIndex = 0;
      }
    }
}
//=============================================================================
//-----------------------------------------------------------------------------
/**
 * 计算磷酸铁锂电池充电状态下(12V系统)剩余容量百分比
 * 采用分段线性模型，更贴近实际电池放电曲线
 * @param voltage 电池当前电压(V)
 * @return 剩余容量百分比(0-100)
 */
static float getLifepo4Capacity(float voltage) 
{

  float fReault;
  // 电压低于放电截止电压，返回0%
  if (voltage <= 10.0f) 
    {
    fReault = 0;
    }
  // 10.0V ~ 12.0V：低电量区域，容量上升较慢
  else if (voltage < 12.0f) 
    {
    fReault = ((voltage - 10.0f) / 2.0f) * 20.0f;
    }
  // 12.0V ~ 13.0V：中等电量区域，容量线性上升
  else if (voltage < 13.0f) 
    {
    fReault = ((voltage - 12.0f) / 1.0f) * 30.0f + 20.0f;
    }
  // 13.0V ~ 14.0V：较高电量区域，容量上升加快
  else if (voltage < 14.0f) 
    {
    fReault = ((voltage - 13.0f) / 1.0f) * 30.0f + 50.0f;
    }
  // 14.0V ~ 15.6V：充电末期，容量缓慢接近100%
  else if (voltage < 15.6f) 
    {
   fReault = ((voltage - 14.0f) / 1.6f) * 20.0f + 80.0f;
    }
  // 电压达到或超过满电电压，返回100%
  else 
    {
    fReault = 100;
    }

  return fReault;
}
//-----------------------------------------------------------------------------
/**
 * 计算磷酸铁锂电池(14.6V额定电压)静置状态下的剩余电量百分比
 * 基于开路电压特性的分段模型，适用于不充电、不放电的静置状态
 * @param openVoltage 电池开路电压(V)
 * @return 剩余电量百分比(0-100)
 */
static float getLifepo4RestCapacity(float openVoltage) 
{

  float fReault;
  // 放电截止电压，低于此值视为0%
  if (openVoltage <= 10.5f) 
    {
    fReault = 0;
    }
  // 10.5V ~ 12.0V：低电量区域，电压上升较慢
  else if (openVoltage < 12.0f) 
    {
    // 1.5V电压跨度对应20%容量
    fReault = ((openVoltage - 10.5f) / 1.5f) * 20.0f;
    }
  // 12.0V ~ 13.2V：中等电量区域，电压与容量线性关系较好
  else if (openVoltage < 13.2f) 
    {
    // 1.2V电压跨度对应35%容量
    fReault = ((openVoltage - 12.0f) / 1.2f) * 35.0f + 20.0f;
    }
  // 13.2V ~ 14.0V：较高电量区域，电压上升加快
  else if (openVoltage < 14.0f) 
    {
    // 0.8V电压跨度对应30%容量
    fReault = ((openVoltage - 13.2f) / 0.8f) * 30.0f + 55.0f;
    }
  // 14.0V ~ 14.6V：满电区域，接近额定电压
  else if (openVoltage < 14.6f) 
    {
    // 0.6V电压跨度对应15%容量
    fReault = ((openVoltage - 14.0f) / 0.6f) * 15.0f + 85.0f;
    }
  // 达到或超过额定电压，视为100%
  else 
    {
    fReault = 100;
    }

  return fReault;
}
//-----------------------------------------------------------------------------
// 读锂电池充电测量
void ADCMDR_ReadBatChargerMeasure(void)
{
  
  TINA226Measure mResult;
  int iRes = INA226_ReadMeasure( 0, &mResult );
  
  if( 0 == iRes )
    {
    // ---------
    float fValue = ((short)mResult.uwVoltShunt) * 2.5 * 0.001;  // mV
    if( fValue < 0 )
      fValue = -fValue;
    _SetRealReg( REG_RL_BCHRG_Ushunt, fValue ); // mV

    // ---------
    float fIBus = ((short)mResult.uwCurrent) * 0.2f; //电流的LSB = 0.2mA，由用户配置
    if( 4.9999f < fIBus ) // 超5mA视为在充电
      SetMLEDState( RMLED_Charging );
    else
      {
      ClrMLEDState( RMLED_Charging );
      }
    _SetRealReg( REG_RL_BCHRG_Ibus, fIBus ); // mA

    auto fIBusMax = _GetRealReg( REG_RL_BCHRG_Ibus_Max );
    if( fIBusMax < fIBus )
      _SetRealReg( REG_RL_BCHRG_Ibus_Max, fIBus ); // mA

    // ---------
    fValue = mResult.uwPower * 0.2f * 25;         // 功率的LSB = 电流的LSB * 25
    _SetRealReg( REG_RL_BCHRG_Pbus, fValue );

    // ---------电池电压
    auto fVbat = mResult.uwVoltBus * 0.00125f;    //电压的LSB = 1.25mV
    _SetRealReg( REG_RL_BCHRG_Ubus, fVbat );
    
    // ---------计算电池电量
    // 是否在充电状态
    if( 0 != Adapter_ON_State && 4.9999 < fIBus )
      {
      // 计算磷酸铁锂电池充电(15.6V系统)状态下的剩余容量百分比
      fValue = getLifepo4Capacity( fVbat );
      }
    else
      {
      // 计算磷酸铁锂电池(14.6V额定电压)静置状态下的剩余电量百分比
      fValue = getLifepo4RestCapacity( fVbat );
      }

    if( 100 < fValue )
      fValue = 100;
    else if( 98 < fValue )
      SetMLEDState(RMLED_Charged);
    else
      ClrMLEDState(RMLED_Charged);

    _SetRealReg( REG_RL_BCHRG_Level, fValue);
    }
  else
    {
    SetHWFault( RHF_ExADC1_ERR );
    }
    
  if( RHF_ExADC1_ERR == GetHWFault( RHF_ExADC1_ERR ) && 0 == iRes )
    {
    uint32_t uINA226Id = 0;
    iRes = INA226_ReadIdent(0, &uINA226Id);
    if( 0x54492260 == uINA226Id || 0 == iRes )
      ClrHWFault( RHF_ExADC1_ERR );
    }
}
//-----------------------------------------------------------------------------
// 读锂电池放电测量
void ADCMDR_ReadBatOutputMeasure(void)
{

  TINA226Measure mResult;
  int iRes = INA226_ReadMeasure( 1, &mResult );

  if( 0 == iRes )
    {
    // ---------
    float fValue = ((short)mResult.uwVoltShunt) * 2.5 * 0.001;  // mV
    if( fValue < 0 )
      fValue = -fValue;
    _SetRealReg( REG_RL_BTOUT_Ushunt, fValue );   // mV

    // ---------
    fValue = ((short)mResult.uwCurrent) * 1.0f;   //电流的LSB = 1mA，由用户配置
    if( 100.0f > fValue )
      fValue = 0.0;    // 去零飘
    _SetRealReg( REG_RL_BTOUT_Ibus, fValue );     // mA

    auto fIBusMax = _GetRealReg( REG_RL_BTOUT_Ibus_Max );
    if( fIBusMax < fValue )
      _SetRealReg( REG_RL_BTOUT_Ibus_Max, fValue ); // mA

    // ---------
    fValue = mResult.uwPower * 0.25f * 25;        // 功率的LSB = 电流的LSB * 25
    _SetRealReg( REG_RL_BTOUT_Pbus, fValue );     // mV

    // ---------
    fValue = mResult.uwVoltBus * 0.00125f;        //电压的LSB = 1.25mV
    _SetRealReg( REG_RL_BTOUT_Ubus, fValue );     // mV
    }
  else
    {
    SetHWFault( RHF_ExADC2_ERR );
    }

  if( RHF_ExADC2_ERR == GetHWFault( RHF_ExADC2_ERR ) && 0 == iRes )
    {
    uint32_t uINA226Id = 0;
    iRes = INA226_ReadIdent(1, &uINA226Id);
    if( 0x54492260 == uINA226Id || 0 == iRes )
      ClrHWFault( RHF_ExADC2_ERR );
    }
}
//-----------------------------------------------------------------------------
// 读锂电池测量
void ADCMDR_ReadBatMeasure(void)
{

  // 读锂电池充电测量
  ADCMDR_ReadBatChargerMeasure();

  // 读锂电池放电测量
  ADCMDR_ReadBatOutputMeasure();
} 
//-----------------------------------------------------------------------------
// 读RTC测量
void ADCMDR_ReadRTCMeasure(void)
{

  int iValue;
  int iRes = SD3077_GetTemperature( &iValue );
  if( 0 == iRes )
    {
    _SetRealReg( REG_RL_RTC_TEMP, iValue );
    }

  iRes = SD3077_ReadBVol( &iValue );
  if( 0 == iRes )
    {
    _SetRealReg( REG_RL_RTC_Vbat, (float)iValue / 10.0 );
    }
}
//-----------------------------------------------------------------------------
