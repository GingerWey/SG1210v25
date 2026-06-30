//-----------------------------------------------------------------------------
/*
 File        : DevFunc.c
 Version     : V1.01
 By          : 银网科技

 Description :实现装置功能的管理

 Date       : 2017.8.19

  2024.1.20
  v3.0
    定义SG1210功能接口
*/
//-----------------------------------------------------------------------------
#include "DevFunc.h"

#include "DevRegs.h"

#include "DevDebug.h"


#include "FuncUVTC.h"

#if METERING_EN > 0
  #include "Cumulant.h"
#endif

#include <string.h>
#define DBG_TIME_MEASURE 0
#if DBG_TIME_MEASURE > 0
 #include "tim.h"
#endif
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define  ACHL_Uin           9  //ADC_CHANNEL_10
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法引用
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
#if NUM_DEV_FUNCs > 1
  const TConstDevFuncInterfacePtr FuncInterface[] =
  {
     &dfiFuncXL           // XL
    ,&dfiFuncCX           // CX
  };
  #define NUM_FUNC_INTFs              NUM_Elements(FuncInterface)

#else
  #define FuncInterface   dfiFuncUVTC
#endif
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void DevFunc_Init(void)
{

//  CMBuf_Init();

#if NUM_DEV_FUNCs > 1
  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->InitProc,
              GFC_EmptyPtr );
 #endif

  pInterface->InitProc();
#else
  FuncInterface.InitProc();
#endif
}
//-----------------------------------------------------------------------------
// 保护数据计算（实时）
void DevFunc_ProtectCalc(void)
{

  // 装置不在“正常”状态，不做保护数据
  TDevStateReg dsDevState = GetDevMode;
  if( RDM_NORMAL != dsDevState &&
      RDM_CALIB  != dsDevState &&
      RDM_TEST   != dsDevState )
    return;

#if NUM_DEV_FUNCs > 1
  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->ProtectCalc,
              GFC_EmptyPtr );
 #endif

  pInterface->ProtectCalc();
#else
  FuncInterface.ProtectCalc();
#endif
}
//-----------------------------------------------------------------------------
// 测量数据计算
void DevFunc_MeasureCalc(void)
{

  // 装置不在“正常”状态，不做测量数据计算
  TDevStateReg dsDevState = GetDevMode;
  if( RDM_NORMAL != dsDevState &&
      RDM_CALIB  != dsDevState &&
      RDM_TEST   != dsDevState )
    return;

#if NUM_DEV_FUNCs > 1
  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->MeasureCalc,
              GFC_EmptyPtr );
 #endif

  pInterface->MeasureCalc();
#else
  
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == FuncInterface.MeasureCalc, GFC_EmptyPtr );
 #endif
  
  FuncInterface.MeasureCalc();
#endif

#if METERING_EN > 0
  // 计算电度
  CUMULANT_Tick();
#endif
}
//-----------------------------------------------------------------------------
// 谐波数据计算
void DevFunc_HarmonicCalc(void)
{

  // 装置不在“正常”状态，不处理谐波数据
  TDevStateReg dsDevState = GetDevMode;
  if( RDM_NORMAL != dsDevState &&
      RDM_CALIB  != dsDevState &&
      RDM_TEST   != dsDevState )
    return;

#if NUM_DEV_FUNCs > 1
  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->HarmonicCalc, 
              GFC_EmptyPtr );
 #endif

  pInterface->HarmonicCalc();
#else
  FuncInterface.HarmonicCalc();
#endif
}
//-----------------------------------------------------------------------------
// 保护逻辑
void DevFunc_LogicProc(void)
{

  // 装置不在“正常”状态，不做逻辑运算
  if( RDM_NORMAL != GetDevMode )
    return;

//#if !defined(__vmSIMULATOR__) && defined(__DEBUG)
//  LED_ST7_ON;
//#endif

#if DBG_TIME_MEASURE > 0
  uint32_t uTMRCnt1 = htim1.Instance->CNT;
#endif

#if NUM_DEV_FUNCs > 1
  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface ||
              nullptr == pInterface->ProtectCalc ||
              nullptr == pInterface->LogicProc, 
              GFC_EmptyPtr );
 #endif

  // 保护数据计算
  pInterface->ProtectCalc();
  // 逻辑运算
  pInterface->LogicProc();
#else
  // 保护数据计算
  FuncInterface.ProtectCalc();

  // 逻辑运算
  FuncInterface.LogicProc();
#endif

#if DBG_TIME_MEASURE > 0
  uint32_t uTMRCnt2 = htim1.Instance->CNT;
  if( uTMRCnt2 > uTMRCnt1 )
    uTMRCnt1 = uTMRCnt2 - uTMRCnt1;
  else
    uTMRCnt1 = uTMRCnt2 + (0xFFFFFFFF - uTMRCnt1) + 1;

  //
 #if ADC_SAMPLES_PER_PEROID < 62
  DevReg_Write( REG_DEV_Probe4, uTMRCnt1 / 90 );
 #else
  DevReg_Write( REG_DEV_Probe4, uTMRCnt1 / 180 );
 #endif
#endif

//#if !defined(__vmSIMULATOR__) && defined(__DEBUG)
//  LED_ST7_OFF;
//#endif
}
//-----------------------------------------------------------------------------
// 填写装置参数
void DevFunc_LoadDefDevConfig(TDeviceFixed* pDevConfig)
{

  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pDevConfig ||
              nullptr == pInterface ||
              nullptr == pInterface->InfoLists, 
              GFC_EmptyPtr );
#endif

  // 允许编辑
  SetEditEnable;

  for( uint32_t uIdx = 0; uIdx < pInterface->InfoListsCount; uIdx++ )
    {
    const TDevRegInfoList *pList =  pInterface->InfoLists[uIdx];
    if( nullptr == pList )
      continue;

    if( REG_DEVOPTION   == REG_TYPE( pList->RegBeg ) ||
        REG_DEVCONFIG   == REG_TYPE( pList->RegBeg ) ||
        REG_DIGINALAIS  == REG_TYPE( pList->RegBeg ) ||
        REG_CALIBRATION == REG_TYPE( pList->RegBeg ) ||
        REG_FUNCTION    == REG_TYPE( pList->RegBeg ) )
      {
      for( uint32_t uItemNo = 0; uItemNo < pList->Count; uItemNo++ )
        {
        const TDevRegInfoItem *pItem = &pList->List[uItemNo];

        switch( REG_TYPE( pItem->RegNum ) )
          {
          case REG_DEVOPTION:
            {
            if( STATE_TRUE == pItem->DefValue )
              pDevConfig->Options |=  (1 << (pItem->RegNum - REG_DEVOPTION));
            else
              pDevConfig->Options &= ~(1 << (pItem->RegNum - REG_DEVOPTION));
            break;
            }

          case REG_DEVCONFIG:
            {
            pDevConfig->Configs[pItem->RegNum - REG_DEVCONFIG] = pItem->DefValue;
            break;
            }
#ifdef CNT_DigInAlais
          case REG_DIGINALAIS:
            {
            pDevConfig->DigInAlais[pItem->RegNum - REG_DIGINALAIS] = pItem->DefValue;
            break;
            }
#endif
          case REG_CALIBRATION:
            {
            pDevConfig->CaliCoef[pItem->RegNum - REG_CALIBRATION] = pItem->DefValue;
            break;
            }

          case REG_FUNCTION:
            {
            if( REG_DEVOPTION == pItem->RefReg ||
                REG_DEVCONFIG == pItem->RefReg )
              DevReg_Write( pItem->RegNum, pItem->DefValue );
            break;
            } // case
          } // switch
        } // for
      } // if
    } // for

  if( nullptr != pInterface->LoadDefDevConfig )
    pInterface->LoadDefDevConfig(pDevConfig);

  // 禁止编辑
  SetEditDisable;

  // 清修改标志
  ClrGUIState( REM_CFG_MODIFIED ); //| REM_SWT_MODIFIED | REM_HLD_MODIFIED );
}
//-----------------------------------------------------------------------------
//// 填写默认定值
//void DevFunc_LoadDefHolding(void)
//{

//  // 取当前功能类型接口
//  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
//#ifdef USE_DEV_ASSERT
//  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->InfoLists,
//              GFC_EmptyPtr );
//#endif

//  // 把默认压板、定值填入编辑区
//  for( uint32_t uIdx = 0; uIdx < pInterface->InfoListsCount; uIdx++ )
//    {
//    const TDevRegInfoList *pList =  pInterface->InfoLists[uIdx];
//    if( nullptr == pList )
//      continue;

//    if( REG_SWITCH  == REG_TYPE( pList->RegBeg ) ||
//        REG_HOLDING == REG_TYPE( pList->RegBeg ) )
//      {
//      for( uint32_t uItemNo = 0; uItemNo < pList->Count; uItemNo++ )
//        {
//        const TDevRegInfoItem *pItem = &pList->List[uItemNo];

//        switch( REG_TYPE( pItem->RegNum ) )
//          {
//          // 压板
//          case REG_SWITCH:
//            {
//            EditHolding.Data.Switch[pItem->RegNum - REG_SWITCH] = pItem->DefValue;
//            break;
//            }

//          // 定值
//          case REG_HOLDING:
//            {
//            EditHolding.Data.Data[pItem->RegNum - REG_HOLDING] = pItem->DefValue;
//            break;
//            }
//          }
//        }
//      }
//    }

//  // 让不同类型填写专有
//  if( pInterface->LoadDefHolding )
//    pInterface->LoadDefHolding();
//}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 获取当前功能接口
const TDevFuncInterface* DevFunc_CurFuncInterface(void)
{

#if NUM_DEV_FUNCs > 1
  if( 0xFF == DevConfig.DevFunc )
    DevConfig.DevFunc = 0;

 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( NUM_FUNC_INTFs < NUM_DEV_FUNCs, GFC_SystemErr );
 #endif

 if( NUM_DEV_FUNCs <= DevConfig.DevFunc )
   {
 #ifdef USE_DEV_ASSERT
    DEV_FAULT( GFC_ErrParam );
 #endif

    return nullptr;
    }

 return FuncInterface[DevConfig.DevFunc];

#else
   return &FuncInterface;
#endif
}
//-----------------------------------------------------------------------------

