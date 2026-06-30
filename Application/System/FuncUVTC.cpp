//-----------------------------------------------------------------------------
/*
 File        : FuncUVTC.cpp
 Version     : V1.01
 By          : 银网科技

 Description :实现SUTC的调用接口

 Date       : 2017.9.10

  2024.1.20
  v3.0
    定义SG1210功能接口
*/
//-----------------------------------------------------------------------------
#include "FuncUVTC.h"

#include "DevRegs.h"
#include "DevRegInfoList.h"

#include "NumProc.h"
#include "Strings/TextStrs.h"

#ifdef __vmSIMULATOR__
  //#include "fft.h"
#endif

#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 默认的刻度系数
static constexpr uint16_t  _CalibCoef  [CNT_Calibration] =
{
  6870, 6870, 0, 0  //
};
//=============================================================================
// 数据引用
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 寄存器信息列表
static constexpr TConstDevRegInfoListPtr _RegInfosList[] =
{
   &SUTCProtDataRegInfoList         // 0 rcProtMeasure 保护数据寄存器描述表
  ,&SUTCMeasDataRegInfoList         // 1 rcACMeasure   测量数据寄存器描述表
  ,nullptr                          // 2 rcMetering    电度计数寄存器描述表
  ,nullptr                          // 3 rcHarmonic    谐波数据寄存器描述表
  ,&StateDigitaInputRegInfoList     // 4 rcBinaryIn    开入状态寄存器描述表
  ,nullptr                          // 5 rcBinInFunc   开入功能状态寄存器描述表
  ,&StateRelayOutputRegInfoList     // 6 rcBinaryOut   开出状态寄存器描述表
  ,&ProtSignRegInfoList             // 7 rcProtSignals 软遥信寄存器描述表

  ,&EventReportRegInfoList          // 8 rcEvents      设备级事件寄存器描述表
  ,&DevOptionRegInfoList            // 9 rcDevOption   设备配置寄存器描述表
  ,&DevConfigRegInfoList            // 10rcDevConfig   设备参数寄存器描述表
  ,nullptr                          // 11rcDIAlais     开入别名寄存器描述表
  ,nullptr                          // 12rcProtEnables 软压板寄存器描述表
  ,nullptr                          // 13rcProtSet     保护定值寄存器描述表
  ,&DevFunctionRegInfoList          // 14rcFunction    设备级功能寄存器描述表
  ,nullptr                          // 15rcSysState    系统状态寄存器

  ,nullptr                          // 16rcMeasAdjust  模拟量手调寄存器描述表
  ,nullptr                          // 17rcMeasTest    模拟量传动寄存器描述表
  ,nullptr                          // 18rcDITest      开入传动寄存器描述表
  ,nullptr                          // 19rcDOTest      开出传动寄存器描述表
};
#define NUM_RegInfoList             NUM_Elements(_RegInfosList)
//-----------------------------------------------------------------------------
////
//static constexpr TRemoteCtrlItem _RemoteItems[] =
//{
//  // RelayIdx  DigInReg  State
//  {        0,  REG_DI3,  STATE_TRUE }
// ,{        1,  REG_DI4,  STATE_TRUE }
//};
//#define NUM_RemoteItems             NUM_Elements(_RemoteItems)
////
//static constexpr TRemoteCtrlList _RemoteCtrlList =
//{
//  0,                                // LocalLatchReg 就地操作闭锁信号寄存器
//  0,                                // ServiceReg    检修操作信号寄存器
//  NUM_RemoteItems,                  // Count         Items数量
//  _RemoteItems                      // Items         控制项
//};
//-----------------------------------------------------------------------------
//static const TADC3Ranks _LVCCADCRanks[] =
//{
//  { // 第1轮
//    { // ADC1
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin
//    },
//    { // ADC2
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin
//    },
//    { // ADC3
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin,
//      ACHL_Uin,   ACHL_Uin,   ACHL_Uin,   ACHL_Uin
//    }
//  }
//};
//#define NUM_LVCCCSQEs      NUM_Elements(_LVCCADCRanks)
//// XL的通道定义
//const TADCRanksDef LVCCADCRanksDef = 
//{
//   NUM_LVCCCSQEs               // NbrOfSqs
//  ,NUM_ADC_Conversions         // NbrOfRanks
//  ,&_LVCCADCRanks              // Ranks
//  // 0=这个通道没有安排采样 1=这个通道采了1次 0x81=这是直流通道
//  ,{ 16 }  
//  // 通道对应的有效值寄存器
//  ,{ REG_CM_UinVal }
//};
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0
// LVCC的录波轨定义
//-----------------------------------------------------------------------------
// 模拟录波轨
static const TWaveLogTrackDef _pADCTracksDef[] =
{
  // RegisterNum  InputChannel Coefficient Name
  { REG_CM_UinVal,  ACHL_Uin,     2263,      0  },
 ,{ REG_CM_UoutVal, ACHL_Uout,    2263,      0  },
};
#define Num_ADCTracksDef              NUM_Elements(_pADCTracksDef)
//-----------------------------------------------------------------------------
// 数字录波轨
static const TWaveLogTrackDef _pBINTracksDef[] =
{
  // RegisterNum      InputChannel Coefficient  Name
  { REG_DI0,              0,        0,           0      },
  { REG_DI1,              0,        0,           0      },
  { REG_DI2,              0,        0,           0      },
  { REG_DI3,              0,        0,           0      },
  { REG_DI4,              0,        0,           0      },
  { REG_DI5,              0,        0,           0      },
  { REG_DI6,              0,        0,           0      },
  { REG_DI7,              0,        0,           0      }
};
#define Num_BINTracksDef              NUM_Elements(_pBINTracksDef)
//-----------------------------------------------------------------------------
const TWaveLogDef LVCCWaveLogDef =
{
   Num_ADCTracksDef   // NbrOfADCTracks 模拟录波轨数量
  ,Num_BINTracksDef   // NbrOfBINTracks 数字录波轨数量

  ,_pADCTracksDef     // ADCTracks 模拟量录波轨定义数据
  ,_pBINTracksDef     // BINTracks 数字量录波轨定义数据
};
#endif
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 初始化
static void funcInit(void)
{

#ifdef __vmSIMULATOR__
  //Init_FFT();
#endif

  if( dfiFuncUVTC.LogicReset )
    dfiFuncUVTC.LogicReset();
}
//-----------------------------------------------------------------------------
// 填写装置参数
static void _loadDefDevConfig(TDeviceFixed* pDevConfig)
{

#ifdef CNT_TripMatrix
  // Wey. 填写到DevConfig的编辑区
  memcpy( (void*)pDevConfig->TripMatrix,
          (void*)CMTripMartix.Default,
          sizeof(pDevConfig->TripMatrix) );
#endif

  //填写刻度系数默认值
  for( uint32_t uIdx = 0; uIdx < CNT_Calibration; uIdx++ )
    {
    if(0 == pDevConfig->CaliCoef[uIdx])
      pDevConfig->CaliCoef[uIdx] = _CalibCoef[uIdx];
    }
}
//-----------------------------------------------------------------------------
// 搜索事件属性
static const TEventProperty* _getRegEventProperty(uint32_t uRegNum)
{

  const TEventProperty *pItem = nullptr;
  for( uint32_t uIdx = 0; uIdx < SUTCEventPropertys.Count; uIdx++ )
    {
    if( SUTCEventPropertys.List[uIdx].Ident == uRegNum )
      {
      pItem = SUTCEventPropertys.List + uIdx;

      break;
      }
    }

  return pItem;
}
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
// LVCC功能接口
const TDevFuncInterface dfiFuncUVTC =
{
   DEVFUN_UVTC             // Type        功能代码
  ,idDevFuncUVTC           // Name        功能名称

  ,funcInit                // InitProc    初始化接口

  ,NumProc_ProtectCalc     // ProtectCalc 保护数据处理接口
  ,NumProc_MeasureCalc     // MeasureCalc 测量数据处理接口
  ,nullptr                 // HarmonicCalc 谐波计算处理接口

  ,nullptr                 // LogicReset  保护逻辑复位
  ,nullptr                 // LogicProc   逻辑运算接口

  ,_loadDefDevConfig       // LoadDefDevConfig 填写装置参数
  ,nullptr                 // LoadDefHolding   填写默认定值

  ,_getRegEventProperty    // GetEventProperty 取TEventProperty

  ,NUM_RegInfoList         // InfoListCount    寄存器信息描述表数量
  ,_RegInfosList           // InfoLists        寄存器信息描述表数量

  ,nullptr                 // ADCRanks         驱动ADC采样的通道定义
#if WAVELOGGER_EN > 0
  ,&XLWaveLogDef           //                  录波通道定义
#endif

#ifndef __vmSIMULATOR__
  ,nullptr                 // CommInfos;       通信点表
  ,nullptr                 // RmtCtrlLiist     遥控表
#endif

#ifdef CNT_TripMatrix
  ,nullptr                 // TripMatrixDef    控制矩阵
#endif
};
//=============================================================================
// 全局数据引用
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
