//-----------------------------------------------------------------------------
/*
 File        : DevIntf.h
 Version     : V1.01
 By          : 卫荣平

 Description :实现访问系统功能的接口
              TDevInterface 可作为GUI/COM的基类，提供直接访问系统功能的接口

 Date       : 2017.12.6
*/
//-----------------------------------------------------------------------------
#include "DevIntf.h"

#include "DevBuffer.h"
#include "DevEvtMgr.h"
#include "DevFixed.h"
#include "DevFunc.h"
#include "DevRegs.h"
#include "DevTypes.h"
#include "Dev_Cfg.h"

#if WAVELOGGER_EN > 0
 #include "WaveLogger.h"
#endif

//#include "Relay.h"
#ifdef __vmSIMULATOR__
   #include <Windows.h>
 #ifdef __QT__
   #include "qdatetime.h"
 #endif
#else
  #include "dio.h"
  #include "rtc.h"
  #include "iwdg.h"
  #include "flash_if.h"
  #include "cmsis_os.h"
  #include <stm32f4xx_hal.h>
#endif

#include <string.h>
#include <cstdint>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 看门狗                 0x0080         0x0100         0x0200
#define RSS_IWDGRTN    (RRS_ADCTASK | RRS_TMRTASK | RRS_GUITASK | RRS_CTRLTASK) // | RRS_UARTTASK)
#define GetIWDGRTN     ( (GetRSTSrc(RSS_IWDGRTN) == RSS_IWDGRTN)? STATE_TRUE : STATE_FALSE )
#define ClrIWDGRTN     ( DevCache.DeviceState[REG_RST_SOURCE - REG_DEVSTATE] &= ~(RSS_IWDGRTN) )
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
#ifndef __vmSIMULATOR__
  // 使能看门狗
  #define IWDG_ENABLE    1
#else
  // 使能看门狗
  #define IWDG_ENABLE    0     // 模拟器下没有看门狗
#endif
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 上次搜索到的 RegInfoList 指针
static const TDevRegInfoList* pLastRegInfoList = nullptr;
//=============================================================================
// 全局变量引用
//-----------------------------------------------------------------------------
// 装置参数区的编辑区
extern TDeviceFixed DevCfgForEdit;
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void DevIntf_Init(void)
{
  pLastRegInfoList = nullptr;
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 装置功能类型
//-----------------------------------------------------------------------------
// 取装置功能类型
// 输入：（无）
// 输出：（无）
// 返回：（无）
uint8_t DevIntf_GetDevFunc(void)
{

  return DevConfig.DevFunc;   // 读装置类型寄存器
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// MCU访问
//-----------------------------------------------------------------------------
// 读取MCU状态
uint32_t DevIntf_GetMCUState(void)
{

#ifdef __vmSIMULATOR__
  return 0; //(MCU_WDTOK | MCU_FRONLY | MCU_BOOT);
#else
  uint32_t uRes = 0;

  // 读 RDP 状态
  uint32_t uRDP_Level = FLASH_If_GetReadProtection();
  if( OB_RDP_LEVEL_1 == uRDP_Level ||
      OB_RDP_LEVEL_2 == uRDP_Level )
    uRes |= MCU_FRONLY;

  if( 0 != (IWDG->PR & 0x7) )
    uRes |= MCU_WDTOK;

  if( (uint32_t)SCB->VTOR - (uint32_t)FLASH_BASE >= 0x10000 )
    uRes |= MCU_BOOT;

#ifndef __DEBUG
//  if( 0 == (uRes & MCU_FRONLY) )
//    {
//    FLASH_If_EnableWriteProtection();
//    FLASH_If_EnableReadProtection();
//    }
#endif

  return uRes;
#endif
}
//-----------------------------------------------------------------------------
// 复位装置
// 由vGUI调用
// 输入:
//   uToken: 令牌
// 输出：(无)
// 返回：(无)
void DevIntf_ResetMCU(uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrParam );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return ;
#endif

#ifdef __vmSIMULATOR__
  DevReg_Init();

  FIX_Init();

  //GUIAppPort_OnReset();
  //GUIAppPort_OnStartup();
#else
  osDelay( 100 );

  __disable_irq();
  while( 1 )
    HAL_NVIC_SystemReset();  //复位系统
  #endif
}
//-----------------------------------------------------------------------------
// 重新配置装置硬件
// 由GUI调用
//   1.GUI对某些外设（如UART\NIC等）参数做过设置，需要重新配置这些外设
//   2.GUI对某些外设（如DI\DO等）进行了测试，需要恢复
// 输入：（无）
// 输出：（无）
// 返回：（无）
void DevIntf_PeripheralsReconfig(void)
{

  // 从“测试”状态返回
  if( RDM_TEST == GetDevMode )
    {
    //
    const TDevRegInfoList* pMeasList = DevIntf_GetRegsList( rcMeasTest );
    if( nullptr != pMeasList )
      {
      // 清模拟量
      for( int32_t iIdx = pMeasList->Count - 1; iIdx >= 0; iIdx-- )
        DevReg_Write( pMeasList->List[iIdx].RefReg, 0 );
      }

    // 要求 恢复开入开出
    SetSetHW( RSH_DI | RSH_DO );

    // 进入正常状态
    SetDevMode( RDM_NORMAL );
//    ClrGUIState(RGUI_LAMPS_TEST);
    }
  else if( RDM_CALIB == GetDevMode )
    {
    // 恢复设备参数
    FIX_LoadDevConfig();

    // 进入正常状态
    SetDevMode( RDM_NORMAL );
    }

//  // 恢复开入开出
//  if( GetSetHW(RSH_DI | RSH_DO) )
//    {
//    //  禁止[启动继]
//    Relay_DOEActive( TOKEN_RELAY_OFF );

//    // 关闭开出
//    DIO_RelaysAllOff( TOKEN_RELAY_OFF );
//    // 重置开入状态和状态机
//    DIO_Cover();

//    // 清 请求
//    ClrSetHW( RSH_DI | RSH_DO );
//    }

//#ifndef __vmSIMULATOR__
//  // 禁止任务切换
//  osThreadSuspendAll();
//#endif

//  if( GetSetHW(RSH_NIC1) ||
//      GetSetSocket(RSS_SOCKET11 | RSS_SOCKET12 | RSS_SOCKET13 | RSS_SOCKET14) )
//    {
//    // 交给网口1任务去处理
//    SetTodoTask( RTT_NIC1_RECONFIG );
//    }

//  if( GetSetHW(RSH_NIC2) ||
//      GetSetSocket(RSS_SOCKET21 | RSS_SOCKET22 | RSS_SOCKET23 | RSS_SOCKET24) )
//    {
//    // 交给网口2任务去处理
//    SetTodoTask( RTT_NIC2_RECONFIG );
//    }

//  if( GetSetHW(RSH_UART1 | RSH_UART1_PROTOCOL ) )
//    {
//    // 交给串口1任务去处理
//    SetTodoTask( RTT_UART1_RECONFIG );
//    }

//  if( GetSetHW(RSH_UART2 | RSH_UART2_PROTOCOL ) )
//    {
//    // 交给串口2任务去处理
//    SetTodoTask( RTT_UART2_RECONFIG );
//    }

//#ifndef __vmSIMULATOR__
//  // 允许任务切换
//  osThreadResumeAll();
//#endif
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 寄存器访问
//-----------------------------------------------------------------------------
static
const TDevRegInfoItem*
searchRegInfoList( const TDevRegInfoList *pRegList,
                                  uint32_t  uRegNum )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pRegList, GFC_ErrParam  );
#endif

  // 用列表中的寄存器范围 粗判是否可能在此列表
  if( pRegList->RegBeg > uRegNum || pRegList->RegEnd < uRegNum )
    return nullptr;  // 不在

  const TDevRegInfoItem* pItem = nullptr;

  // 用二分搜索
  uint32_t uBeg = 0, uEnd = pRegList->Count - 1,
           uPos = uRegNum - pRegList->RegBeg;
  if( uPos > uEnd )
    uPos = (uBeg + uEnd) >> 1;

  while( uEnd - uBeg > 2 )
    {
    if( pRegList->List[uPos].RegNum > uRegNum )
      uEnd = uPos;
    else if( pRegList->List[uPos].RegNum < uRegNum )
      uBeg = uPos;
    else
      {
      pItem = &(pRegList->List[uPos]);

      break;
      }

    uPos  = (uBeg + uEnd) >> 1;
    }
  // 不放过临界点
  if( nullptr == pItem )
    {
    if( pRegList->List[uPos].RegNum == uRegNum )
      pItem = &(pRegList->List[uPos]);
    else if( pRegList->List[uBeg].RegNum == uRegNum )
      pItem = &(pRegList->List[uBeg]);
    else if( pRegList->List[uEnd].RegNum == uRegNum )
      pItem = &(pRegList->List[uEnd]);
    }

//  // 若失败，用线性搜索； 可能寄存器的顺序不对
//  if( 0 == pItem )
//    {
//    //
//    for( int iIdx = pRegList->Count - 1; iIdx >= 0; iIdx-- )
//      {
//      if( pRegList->List[iIdx].RegNum == uRegNum )
//        {
//        pItem = &(pRegList->List[iIdx]);
//
//        break;
//        }
//      }
//    }
    // 再找不到就没有了

  return pItem;
}
//-----------------------------------------------------------------------------
// 获取指定寄存器的描述信息
const TDevRegInfoItem* DevIntf_GetRegInfo(uint32_t uRegNum)
{

  // 取当前功能类型接口
  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface, GFC_ErrRegNum  );
#endif

  const TDevRegInfoItem* pItem;
  // 在上次搜索成功的列表中搜索
  if( nullptr != pLastRegInfoList )
    {
    pItem = searchRegInfoList( pLastRegInfoList, uRegNum );
    if( nullptr != pItem )
      return pItem;
    }
  else
    pItem = nullptr;

  // 遍历接口中定义的各个寄存器描述表
  for( int iIdx = (int)pInterface->InfoListsCount - 1; iIdx >= 0 ; iIdx-- )
    {
    const TDevRegInfoList* pList = pInterface->InfoLists[iIdx];
    // 判定表是否有效
    if( nullptr == pList || 0 == pList->Count || nullptr == pList->List )
      {
      continue ;
      }

    // 搜索当前列表
    pItem = searchRegInfoList( pList, uRegNum );
    if( nullptr != pItem )
      {
      // 找到了
      pLastRegInfoList = pList;

      break;
      }
    }

  return pItem;
}
//-----------------------------------------------------------------------------
// 获取寄存器属性列表
const TDevRegInfoList* DevIntf_GetRegsList(TDevRegListClass regClass)
{

  const TDevFuncInterface *pFuncIntf = DevFunc_CurFuncInterface();

  uint32_t uRegClass = (uint32_t)regClass;
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pFuncIntf ||
              0       == pFuncIntf->InfoListsCount ||
              nullptr == pFuncIntf->InfoLists ||
              0       == uRegClass ||
              pFuncIntf->InfoListsCount < uRegClass,
              GFC_ErrParam  );
#endif

  const TDevRegInfoList* pList   = pFuncIntf->InfoLists[ uRegClass - 1 ];

  return pList;
}
//-----------------------------------------------------------------------------
// 取TEventProperty
const TEventProperty* DevIntf_GetEvtProp(uint32_t uEvtType)
{

  const TDevFuncInterface *pInterface = DevFunc_CurFuncInterface();
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pInterface || nullptr == pInterface->GetEventProperty,
              GFC_ErrParam );
#endif

  return pInterface->GetEventProperty( uEvtType );
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 参数维护
//-----------------------------------------------------------------------------
// 允许定值修改
// 输入: (无)
// 输出：(无)
// 返回：(无)
void DevIntf_ServiceEnable(void)
{
  SetEditEnable;
}
//-----------------------------------------------------------------------------
// 禁止定值修改
// 输入: (无)
// 输出：(无)
// 返回：(无)
void DevIntf_ServiceDisable(void)
{

  SetEditDisable;
}
//-----------------------------------------------------------------------------
// 确认定值修改
// 输入: (无)
// 输出：(无)
// 返回：(无)
void DevIntf_ServiceAccept(void)
{

  // 校准数据被修改
  if( GetEditState( REM_CALIB_MODIFIED ) )
    {
    // 更新到编辑区
    // 不保证 编辑区的内容都有效，要全覆盖
    memcpy( (void*)&DevCfgForEdit, (void*)&DevConfig,  sizeof(DevConfig) );

    // 保存到NvRAM
    if( 0 == FIX_SaveDevConfig( 0 ) )
      {
      // 应用到运行数据
      memcpy(&DevConfig, &DevCfgForEdit, sizeof(DevCfgForEdit));

      // 发送事件
      EVTMGR_AppendEvent( REG_EO_SETDEVCFG, STATE_TRUE );
      }
    else
      // 恢复备份数据
      memcpy(&DevConfig, &DevCfgForEdit, sizeof(DevCfgForEdit));

    ClrEditState( REM_CALIB_MODIFIED );
    }

  // 设备参数有修改
//  if( GetEditState( REM_CFG_MODIFIED   | REM_FUN_MODIFIED |
//                    REM_UART1_MODIFIED | REM_CMX_MODIFIED ) )
  if( GetEditState( REM_CFG_MODIFIED   | REM_UART1_MODIFIED ) )
    {
    // 设备配置
    if( DevCfgForEdit.Options != DevConfig.Options )
      {
      uint32_t uMask = 1, uDiff = (DevCfgForEdit.Options ^ DevConfig.Options);
      for( uint32_t uIdx = 0; uIdx < CNT_DevOptBits; uIdx++ )
        {
        if( 0 != (uDiff & uMask) )
          {
          // 发送事件
          uint8_t ucState;
          if( 0 != (DevCfgForEdit.Options & uMask) )
            ucState = STATE_TRUE;
          else
            ucState = STATE_FALSE;
          EVTMGR_AppendOpLog( REG_DEVOPTION + uIdx, ucState );
          }

        uMask <<= 1;
        }
      }

    // 设备参数
    for( uint32_t uIdx = 0; uIdx < CNT_DevConfigs; uIdx++ )
      {
      if( DevCfgForEdit.Configs[uIdx] != DevConfig.Configs[uIdx] )
        {
        // 发送事件
        EVTMGR_AppendOpLog( REG_DEVCONFIG + uIdx,
                            DevCfgForEdit.Configs[uIdx] );
        }
      }

#ifdef CNT_DigInAlais 
    // 开入别名
    for( uint32_t uIdx = 0; uIdx < CNT_DigInAlais; uIdx++ )
      {
      if( DevCfgForEdit.DigInAlais[uIdx] != DevConfig.DigInAlais[uIdx] )
        {
        // 发送事件
        EVTMGR_AppendOpLog( REG_DIGINALAIS + uIdx,
                            DevCfgForEdit.DigInAlais[uIdx] );
        }
      }
#endif

    // 发操作记录
    // 修改了录波器特性
    if( DevConfig.Configs[REG_WAVELOG_PRE - REG_DEVCONFIG] !=
        DevCfgForEdit.Configs[REG_WAVELOG_PRE - REG_DEVCONFIG] )
      {
      DevConfig.Configs[REG_WAVELOG_PRE - REG_DEVCONFIG] =
         DevCfgForEdit.Configs[REG_WAVELOG_PRE - REG_DEVCONFIG];

#if WAVELOGGER_EN > 0
      // 录波器复位
      WAVELOG_Reset();
#endif
      }

    // 保存设置
    if( 0 == FIX_SaveDevConfig( 0 ) )
      {
      // 应用到运行数据
      memcpy(&DevConfig, &DevCfgForEdit, sizeof(DevCfgForEdit));

      if( GetEditState(REM_CFG_MODIFIED) )
        {
        // 发送事件
        EVTMGR_AppendEvent( REG_EO_SETDEVCFG, STATE_TRUE );
        }

//      if( GetEditState(REM_FUN_MODIFIED) )
//        // 发送事件
//        EVTMGR_AppendEvent( REG_EO_SETDEVFUNCTYPE, STATE_TRUE );

#ifdef REM_ETH1_MODIFIED 
      if( GetEditState(REM_ETH1_MODIFIED) || GetEditState(REM_ETH1_MODIFIED) )
        // 发送事件
        EVTMGR_AppendEvent( REG_EO_SETNETCFG, STATE_TRUE );
#endif

//      if( GetEditState(REM_CMX_MODIFIED) )
//        // 发送事件
//        EVTMGR_AppendEvent( REG_EO_SETCTRLMX, STATE_TRUE );
      }
    }

#ifdef CNT_Switchs
  // 保护定值或压板被修改
  if( GetEditState(REM_HLD_MODIFIED | REM_SWT_MODIFIED) )
    {
    // 如果编辑的是当前区，则发记录
    if( GetEditBlock == DevConfig.ActiveHoldingBlock )
      {
      // 比较压板
      for( uint32_t uIdx = 0; uIdx < CNT_Switchs; uIdx++ )
        {
        if( DevHolding.Data.Switch[uIdx] != EditHolding.Data.Switch[uIdx] )
          {
          EVTMGR_AppendOpLog( REG_SWITCH + uIdx, EditHolding.Data.Switch[uIdx] );
          }
        }

      // 比较定值
//      for( uint32_t uIdx = 0; uIdx < CNT_Holdings; uIdx++ )
//        {
//        if( DevHolding.Data.Data[uIdx] != EditHolding.Data.Data[uIdx] )
//          {
//          EVTMGR_AppendOpLog( REG_HOLDING + uIdx, EditHolding.Data.Data[uIdx] );
//          }
//        }
      }

    // 保存当前定值
    if( 0 == FIX_SaveHolding( 0 ) )
      {
      if( GetEditState( REM_SWT_MODIFIED ) )
        // 发送事件
        EVTMGR_AppendEvent( REG_EO_SETHLDSWT, STATE_TRUE );

      if( GetEditState( REM_HLD_MODIFIED ) )
        // 发送事件
        EVTMGR_AppendEvent( REG_EO_SETHLDATA, STATE_TRUE );
      }
    }
#endif

  // 清 修改标识
    ClrEditState((uint32_t)-1);

  // Wey. 2018.12.28
  // 修改通信参数, 重启系统
//  if( GetEditState( REM_CFG_MODIFIED | REM_FUN_MODIFIED |
//                    REM_CMX_MODIFIED | REM_SWT_MODIFIED |
//                    REM_HLD_MODIFIED | REM_UART1_MODIFIED ) )
  if( GetEditState( REM_CFG_MODIFIED| REM_UART1_MODIFIED ) )
    // 重启系统
    DevIntf_ResetMCU( TOKEN_INTF_OPERATE );
}
//-----------------------------------------------------------------------------
// 取消定值修改
// 输入: (无)
// 输出：(无)
// 返回：(无)
void DevIntf_ServiceCancel(void)
{

  // 校准操作取消
  if( GetEditState( REM_CALIB_MODIFIED ) )
    {
    // 恢复备份数据
    memcpy(&DevCfgForEdit, &DevConfig, sizeof(DevCfgForEdit));

    ClrEditState( REM_CALIB_MODIFIED );
    }

  // 设备参数取消
//  if( GetEditState( REM_CFG_MODIFIED | REM_FUN_MODIFIED |
//                    REM_CMX_MODIFIED | REM_SWT_MODIFIED |
//                    REM_HLD_MODIFIED | REM_UART1_MODIFIED ) )
  if( GetEditState( REM_CFG_MODIFIED | REM_UART1_MODIFIED ) )
    {
    memcpy( (uint8_t*)&DevCfgForEdit, &DevConfig, sizeof(DevConfig) );

    ClrSetHW((uint32_t)-1);
    //    ClrSetSocket( -1u );
    }

#ifdef NUM_HoldingBlocks
  // 定值编辑取消
  if( GetEditState(REM_HLD_MODIFIED | REM_SWT_MODIFIED) )
    {
    FIX_LoadEditHolding();
    }
#endif

  // 清各种 修改 状态
    ClrEditState((uint32_t)-1);

  // 清硬件配置要求
    ClrSetHW((uint32_t)-1);
}
//-----------------------------------------------------------------------------
// 查询设定值是否被编辑
// 输入: (无)
// 输出：(无)
// 返回： REM_CFG_MODIFIED\REM_HLD_MODIFIED
uint32_t DevIntf_GetServiceModfied(void)
{

//  return GetEditState( REM_CFG_MODIFIED | REM_FUN_MODIFIED |
//                       REM_CMX_MODIFIED | REM_SWT_MODIFIED |
//                       REM_HLD_MODIFIED | REM_UART1_MODIFIED );
  return GetEditState( REM_CFG_MODIFIED | REM_UART1_MODIFIED );
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 保护定值区管理
//-----------------------------------------------------------------------------
#ifdef NUM_HoldingBlocks
// 获取当前编辑区号
// 输入: (无)
// 输出：(无)
// 返回：当前编辑的定值区序号
uint32_t DevIntf_GetEditBlockNo(void)
{

  return GetEditBlock;
}
//-----------------------------------------------------------------------------
// 切换编辑运行区
// 由vGUI调用
// 输入:
//  uToken:  令牌
//   uDest: 目标定值区
// 输出：(无)
// 返回：
//   0:成功  -1：无效区号  1:运行区重复  2:当前区
int DevIntf_ChangeEditHoldingBlock( uint32_t uToken, uint32_t uDest )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( NUM_HoldingBlocks <= uDest, GFC_ErrParam );
#endif

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return -2;
#endif

  // 切换编辑定值区
  uint32_t uRes = FIX_SwitchEditHolding( TOKEN_FIX_OPERATE, uDest );
  if( 0 == uRes )
    {
    ClrEditState( REM_HLD_MODIFIED | REM_SWT_MODIFIED );
    }

  return (int)uRes;
}
//-----------------------------------------------------------------------------
// 获取当前运行的定值区号
// 输入: (无)
// 输出：(无)
// 返回：当前运行的定值区序号
uint32_t DevIntf_GetActiveBlockNo(void)
{

  return DevConfig.ActiveHoldingBlock;
}
//-----------------------------------------------------------------------------
// 切换当前运行区
// 由vGUI调用
// 输入:
//  uToken:  令牌
//   uDest: 目标定值区
// 输出：(无)
// 返回：
//   0:成功  -1：无效区号  0=成功 >0出错
int DevIntf_ChangeActiveHoldingBlock( uint32_t uToken, uint32_t uDest )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( NUM_HoldingBlocks <= uDest, GFC_ErrParam );
#endif

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return -2;
#endif

  // 切换当前运行区
  uint32_t uRes = FIX_SwitchActiveHolding( TOKEN_FIX_OPERATE, uDest );
  if( 0 == uRes )
    // 发送事件
    EVTMGR_AppendEvent( REG_EO_CHGACTHOLD, STATE_TRUE );

  // 同时切换编辑区
  // 切换编辑定值区
  uRes = FIX_SwitchEditHolding( TOKEN_FIX_OPERATE, uDest );
  if( 0 == uRes )
    {
    ClrEditState( REM_HLD_MODIFIED | REM_SWT_MODIFIED );
    }

  return (int)uRes;
}
//-----------------------------------------------------------------------------
// 当前编辑区的定值恢复到默认值
// 由vGUI调用
// 输入: (无)
// 输出：(无)
// 返回：0:成功   >0: 失败
uint32_t DevIntf_HoldingLoadDefault(uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return 11;
#endif

  // 读默认定值，并保存到NvRAM
  FIX_LoadDefHolding( TOKEN_FIX_OPERATE );

  if( DF_VALID == EditHolding.Valid )
    {
    // 发送事件
    EVTMGR_AppendEvent( REG_EO_DEF_HOLDING, STATE_TRUE );

    // 恢复运行区的定值, 重启系统
    if( DevConfig.ActiveHoldingBlock == GetEditBlock )
      {
      // 重启系统
      DevIntf_ResetMCU( TOKEN_INTF_OPERATE );
      }
    }

  return ( DF_VALID == EditHolding.Valid )? 0 : 10;
}
//-----------------------------------------------------------------------------
// 当前编辑区的定值复制到
// 由vGUI调用
// 输入:
//  uToken:  令牌 必须是 TOKEN_INTF_OPERATE
//   uDest: 目标定值区
// 输出：(无)
// 返回：
//   0:成功  -1：无效区号  1:运行区重复  2:当前区
int DevIntf_HoldingCopyTo( uint32_t uToken, uint32_t uDest )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( NUM_HoldingBlocks <= uDest, GFC_ErrParam );
    return -1;
#endif

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return -2;
#endif

  // 复制定值区
  uint32_t uRes = FIX_SaveHoldingTo( TOKEN_FIX_OPERATE, uDest, 1 );
  if( 0 == uRes )
    // 发送事件
    EVTMGR_AppendEvent( REG_EO_COPYHOLDING, STATE_TRUE );

  return (int)uRes;
}
#endif
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 设备参数管理
//-----------------------------------------------------------------------------
// 设备参数恢复到当前值
// 用设备参数区的值覆盖设备参数编辑区
// 由vGUI调用
// 输入: (无)
// 输出：(无)
// 返回：0:成功   >0: 失败
uint32_t DevIntf_DevCfgEditPropare(uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken,  GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return 11;
#endif

  // 用运行参数覆盖编辑参数
  memcpy( (uint8_t*)&DevCfgForEdit, &DevConfig, sizeof(DevConfig) );

  ClrSetHW((uint32_t)-1);
 // ClrSetSocket( -1u );
  ClrEditState( REM_CFG_MODIFIED   | 
                REM_UART1_MODIFIED | 
                REM_CALIB_MODIFIED );

  return 0;
}
//-----------------------------------------------------------------------------
// 设备参数恢复到默认值
// 由vGUI调用
// 输入: (无)
// 输出：(无)
// 返回：0:成功   >0: 失败
uint32_t DevIntf_DevCfgLoadDefault(uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrToken );
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return 11;
#endif

  // 恢复设备参数到默认值
  FIX_LoadDefDevConfig( TOKEN_FIX_OPERATE );

  if( DevCfgForEdit.Valid == DF_VALID )
    {
    // 发送事件
    EVTMGR_AppendEvent( REG_EO_DEF_DEVCFG, STATE_TRUE );

    // 重启系统
    DevIntf_ResetMCU( TOKEN_INTF_OPERATE );
    }

  return (DevCfgForEdit.Valid == DF_VALID) ? 0 : 10;
}
//_____________________________________________________________________________
//-----------------------------------------------------------------------------
// 时间访问
//-----------------------------------------------------------------------------
// 读取系统时间
// 从系统中读取读取当前时间
// 输入：(无)
// 输出：
//   pDateTime: 指向存贮当前时间的结构指针
// 返回：(无)
void DevIntf_getDateTime(TDateTimeType *pDateTime)
{

#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == pDateTime, GFC_ErrParam );
#endif


#ifndef __vmSIMULATOR__
   RTC_GetTime(pDateTime);
#else
    SYSTEMTIME stLocal;

    // 获取本地时间
    GetLocalTime(&stLocal);
    //// 获取 UTC 时间
    //GetSystemTime(&stUTC);

    pDateTime->Year    = stLocal.wYear;
    pDateTime->Month   = stLocal.wMonth;
    pDateTime->Day     = stLocal.wDay;
    pDateTime->Hours   = stLocal.wHour;
    pDateTime->Minutes = stLocal.wMinute;
    pDateTime->Seconds = stLocal.wSecond;
    pDateTime->MilSeconds = stLocal.wMilliseconds;
#endif
}
//-----------------------------------------------------------------------------
// 读取系统时间
// 由vGUI调用
// 从SFC中读取读取当前时间
// 输入：(无)
// 输出：
//   puwTime: 指向存贮当前时间的空间，不少于 8 Words
// 返回：(无)
void DevIntf_ReadDateTime(uint16_t *puwTime)
{

#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == puwTime, GFC_ErrParam );
#endif


#ifndef __vmSIMULATOR__
   TDateTimeType dt;
   RTC_GetTime(&dt);

   puwTime[0] = dt.Year;
   puwTime[1] = dt.Month;
   puwTime[2] = dt.Day;
   puwTime[3] = dt.Hours;
   puwTime[4] = dt.Minutes;
   puwTime[5] = dt.Seconds;
   puwTime[6] = dt.MilSeconds;
#else
    SYSTEMTIME stLocal;

    // 获取本地时间
    GetLocalTime(&stLocal);
    //// 获取 UTC 时间
    //GetSystemTime(&stUTC);

    puwTime[0] = stLocal.wYear;
    puwTime[1] = stLocal.wMonth;
    puwTime[2] = stLocal.wDay;
    puwTime[3] = stLocal.wHour;
    puwTime[4] = stLocal.wMinute;
    puwTime[5] = stLocal.wSecond;
    puwTime[6] = stLocal.wMilliseconds;
#endif
}
//-----------------------------------------------------------------------------
// 设置系统时间
// 由vGUI调用
// 设置当前时钟到给定日期日间
// 输入：(无)
//   puwTime: 指向存贮当前时间的空间，不少于 7 Words
//    uToken：令牌
// 输出：
// 返回：(无)
void DevIntf_SetDateTime(uint16_t *puwTime, uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_INTF_OPERATE != uToken, GFC_ErrParam);
#else
  if( TOKEN_INTF_OPERATE != uToken )
    return ;
#endif

#ifndef __vmSIMULATOR__
  TDateTimeType dt;
  dt.Year    = puwTime[0];
  dt.Month   = puwTime[1];
  dt.Day     = puwTime[2];
  dt.Hours   = puwTime[3];
  dt.Minutes = puwTime[4];
  dt.Seconds = puwTime[5];

  RTC_SetTime( &dt );
#endif

  // 发送事件
  EVTMGR_AppendEvent( REG_EO_SET_RTC, STATE_TRUE );
}
//-----------------------------------------------------------------------------
// 取系统时脉
// 取系统自上电以来的毫秒数
// 输入：(无)
// 输出：
// 返回：
//   系统自上电以来的毫秒数
uint32_t DevIntf_GetTickCount(void)
{

#ifndef __vmSIMULATOR__
  return HAL_GetTick();
#elif defined(__QT__)
  qint64 uNow = QDateTime::currentMSecsSinceEpoch();
  return (uint32_t)uNow;
 #else
    return (uint32_t)GetTickCount64();
 #endif
}
//-----------------------------------------------------------------------------
// 计算相对时间(ms)
// 计算从uStart到当前的毫秒数
// 输入：
//   uStart: 开始时刻，可由DevIntf_GetTickCount获得
// 输出：
// 返回：
//   从uStart到当前的毫秒数
uint32_t DevIntf_DiffTimeMs(uint32_t uStart)
{

  uint32_t uNow = DevIntf_GetTickCount();

  uint32_t uRes;
  if( uNow > uStart )
    uRes = uNow - uStart;
  else
    uRes = uNow + (0xFFFFFFFF - uStart);

  return uRes;
}
//=============================================================================
// 看门狗
//-----------------------------------------------------------------------------
// 启动
// 输入：(无)
// 输出：(无)
// 返回：(无)
void DevIntf_StartDog(void)
{

#if IWDG_ENABLE > 0
  // 开看门狗
  MX_IWDG_Init();
#endif
}
//-----------------------------------------------------------------------------
// 看门狗 喂狗
// 输入：(无)
// 输出：(无)
// 返回：(无)
void DevIntf_FeedDog(void)
{

#ifndef __vmSIMULATOR__
  // 复位看门狗
  if( STATE_TRUE == GetIWDGRTN )  // 各任务就绪
    {
 #if IWDG_ENABLE > 0
    // 喂狗
    IWDG_FeedDog();
 #endif

    // 清各任务标识
    ClrIWDGRTN;
    }

// #if 1 //#ifdef __DEBUG
//  else
//    RTC_WriteWDGReady( GetRSTSrc(RSS_IWDGRTN) );
// #endif
#endif
}
//-----------------------------------------------------------------------------
