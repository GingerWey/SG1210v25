//-----------------------------------------------------------------------------
/*
 File        : DevFunc.h
 Version     : V1.01
 By          : 银网科技

 Description :定义装置功能类型, 及调用入口

 Date       : 2017.8.19
 
  2018.9.10
  v2.0
   定义GP301XL功能接口

  2024.1.20
  v3.0
    定义SG1210功能接口
*/
//-----------------------------------------------------------------------------
#ifndef DEV_FUNC_H
#define DEV_FUNC_H

#include "DevTypes.h"
#include "DevFixed.h"

#ifndef __vmSIMULATOR__
 #include "COMTypes.h"
#endif

#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
#include <Dev_Cfg.h>
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
#define NUM_DEV_FUNCs           1       // 通用测量的硬件不支持多类型
//-----------------------------------------------------------------------------
// 各功能类型
#define DEVFUN_UVTC             1
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------
// 
typedef void (*PROC_Ticker)(void);
typedef void (*PROC_DevCfg)(TDeviceFixed*);

// 取TEventProperty的方法
typedef const TEventProperty* (*PROC_GetEvtProp)(uint32_t uEvtType);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 各功能类型的定义数据
//-----------------------------------------------------------------------------
// 类型功能索引
typedef struct tagDevFuncInterface
{
    uint8_t      Type;                        // 装置类型标识
    uint16_t     NameId;                      // 类型名称

    PROC_Ticker  InitProc;                    // 初始化方法

    PROC_Ticker  ProtectCalc;                 // 保护数据计算（实时）
    PROC_Ticker  MeasureCalc;                 // 测量数据计算
    PROC_Ticker  HarmonicCalc;                // 谐波数据计算

    PROC_Ticker  LogicReset;                  // 保护逻辑复位方法
    PROC_Ticker  LogicProc;                   // 保护逻辑运算方法
                                              
    PROC_DevCfg  LoadDefDevConfig;            // 填写装置参数
    PROC_Ticker  LoadDefHolding;              // 填写默认定值

    PROC_GetEvtProp GetEventProperty;         // 取TEventProperty的方法
    
    uint32_t     InfoListsCount;              // 寄存器描述信息组数量
    const TConstDevRegInfoListPtr* InfoLists; // 寄存器描述组

    const TADCRanksDef  *ADCRanks;            // 驱动ADC采样的通道定义
#if WAVELOGGER_EN > 0
    const TWaveLogDef   *WaveLog;             // 录波器通道定义
#endif
  
#ifndef __vmSIMULATOR__
    const TCommInfoPackage* CommInfos;        // 通信点表
    const TRemoteCtrlList  *RmtCtrlList;      // 遥控表
#endif

#ifdef CNT_TripMatrix
    const TTripMatrixDef   *TripMatrixDef;    // 出口矩阵定义数据
#endif
} TDevFuncInterface; 
typedef const TDevFuncInterface*   TConstDevFuncInterfacePtr;
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void DevFunc_Init(void);
//-----------------------------------------------------------------------------
// 保护数据计算（实时）
void DevFunc_ProtectCalc(void);
//-----------------------------------------------------------------------------
// 测量数据计算 （内调用 实时保护数据计算）
void DevFunc_MeasureCalc(void);
//-----------------------------------------------------------------------------
// 谐波数据计算
void DevFunc_HarmonicCalc(void);
//-----------------------------------------------------------------------------
// 保护逻辑
void DevFunc_LogicProc(void);
//-----------------------------------------------------------------------------
// 填写默认装置参数
void DevFunc_LoadDefDevConfig(TDeviceFixed* pDevConfig);
//-----------------------------------------------------------------------------
// 获取当前功能接口
const TDevFuncInterface* DevFunc_CurFuncInterface(void);
//-----------------------------------------------------------------------------
// 填写默认定值
void DevFunc_LoadDefHolding(void);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
