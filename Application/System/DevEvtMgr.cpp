//-----------------------------------------------------------------------------
/*
 File        : DevEventMgr.cpp
 Version     : V1.01
 By          : 银网科技

 Description : 事件管理器

   卫荣平
   2017.9.14
*/
//-----------------------------------------------------------------------------
#include "DevEvtMgr.h"

#include "DevFixed.h"
#include "DevIntf.h"
#include "DevRegInfo.h"
#include "DevRegs.h"

#include "DevDebug.h"
#include "Strings/TextStrs.h"

#if WAVELOGGER_EN > 0
 #include "WaveLogger.h"
#endif

#ifndef __vmSIMULATOR__
  #include "rtc.h"
#else
  #include <WTypesbase.h>
  #include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 用于通讯过滤的宏组合
#define NES_COMMs    (NES_CAN_MASK + NES_UART_MASK + NES_NETSKT_MASK)

//#ifndef nullptr
//  #define  nullptr  null
//#endif
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 事故报告缓冲
// 缓冲区
typedef struct tagEventLogCacheItem
{
  uint32_t         NewLogMarks;

  const TDevRegInfoItem* pRegInfo;
  const TEventProperty*  pEvtProp;

  TEventLogItem    EventLog;
} TEventLogCacheItem;
//-----------------------------------------------------------------------------
// 事件缓冲区
typedef struct tagEventLogCache
{
  // 缓冲区指针
  uint16_t           Count;
  uint16_t           Position;

  TEventLogCacheItem Events[SIZE_EVENT_CACHE];
} TEventLogCache;
//-----------------------------------------------------------------------------
// 带__TEventLogCacheItem指针的调用
typedef void* (*PROC_CacheItem)( struct tagEventLogCacheItem *, void* );
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
static TEventLogCache EventLogCache;
static TEventLogCache DispEventLogCache;
//=============================================================================
// 全局方法引用
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#ifdef __vmSIMULATOR__
static void RTC_FillEventTime( TEventLogSummary* pSummary )
{
    SYSTEMTIME stLocal;

    // 获取本地时间
    GetLocalTime(&stLocal);
    //// 获取 UTC 时间
    //GetSystemTime(&stUTC);

    pSummary->Time.Year    = stLocal.wYear;
    pSummary->Time.Month   = stLocal.wMonth;
    pSummary->Time.Day     = stLocal.wDay;
    pSummary->Time.Hours   = stLocal.wHour;
    pSummary->Time.Minutes = stLocal.wMinute;
    pSummary->Time.Seconds = stLocal.wSecond;
    pSummary->Time.milSecs = stLocal.wMilliseconds;
}
#endif
//-----------------------------------------------------------------------------
static void* saveNewEvent( struct tagEventLogCacheItem *pEvent, void* )
{

  // 补充信息
  // 只保存事件寄存器的序号
//  pEvent->EventLog.Summary.State.RegNum = pEvent->pRegInfo->RegNum;

//  if( 0 != (pEvent->pRegInfo->Property & SIT_EVT_Disp) )     // 需要弹报警窗
//   {
//    pEvent->NewLogMarks |= NES_DISP;
//
//    SetTodoTask( RTT_DISP_EVENT );
//    }

//  if( 0 != (pEvent->NewLogMarks & NES_DISP) )               // 需要弹报警窗
//    SetTodoTask( RTT_DISP_EVENT );    直接用DispEventLogCache.Count判

//  if( 0 != (pEvent->pRegInfo->Property & SIT_EVT_Comm) )    // 需要通信发送
//    {
//    pEvent->NewLogMarks |= NES_COMMs;
//    SetNewEvents(NES_COMMs);
//    }

  // ... todo
  switch( SIT_GetEvtType( pEvent->pRegInfo->Property ) )
    {
    // 事件
    case SIT_EVT_Event:
      {
      pEvent->EventLog.Summary.State.Type = mltEvent;
      FIX_SaveEvtLogItem(  mltEvent, &(pEvent->EventLog.Summary) );
      break;
      }
    // 报警
    case SIT_EVT_Alarm:
      {
      pEvent->EventLog.Summary.State.Type = mltAlarm;
      FIX_SaveEvtLogItem(  mltAlarm, &(pEvent->EventLog) );
      break;
      }
    // 事故
    case SIT_EVT_Fault:
      {
      pEvent->EventLog.Summary.State.Type = mltFault;
      FIX_SaveEvtLogItem(  mltFault, &(pEvent->EventLog)  );
      break;
      }

#ifdef SIT_EVT_Current
    // 电流保护
    case SIT_EVT_Current:
      {
      pEvent->EventLog.Summary.State.Type = mltCurProt;
      FIX_SaveEvtLogItem(  mltFault, &(pEvent->EventLog)  );
      break;
      }
#endif

#ifdef SIT_EVT_RmtCtrl
    // 遥控
    case SIT_EVT_RmtCtrl:
      {
      pEvent->EventLog.Summary.State.Type = mltRmtCtrl;
      FIX_SaveEvtLogItem(  mltEvent, &(pEvent->EventLog)  );
      break;
      }
#endif

#ifdef SIT_EVT_Logic
    // 逻辑
    case SIT_EVT_Logic:
      {
      pEvent->EventLog.Summary.State.Type = mltLogic;
      FIX_SaveEvtLogItem(  mltAlarm, &(pEvent->EventLog)  );
      break;
      }
#endif
    }

  pEvent->NewLogMarks &= ~NES_SAVE;

  return nullptr;
}
//-----------------------------------------------------------------------------
static void* countEvents( struct tagEventLogCacheItem *pEvent, void* pvParam )
{

  (void)pEvent;

  uint32_t* puCount = (uint32_t*)pvParam;

  *puCount = *puCount + 1;

  return nullptr;
}
//-----------------------------------------------------------------------------
static void* clearMark( struct tagEventLogCacheItem *pEvent, void* pvParam )
{

#ifndef __vmSIMULATOR__
  uint32_t uFilter = (uint32_t)pvParam;
#else
  uint64_t uFilter = (uint64_t)pvParam;
#endif

  pEvent->NewLogMarks &= ~uFilter;

  return nullptr;
}
//-----------------------------------------------------------------------------
static void* fetchEvent( struct tagEventLogCacheItem *pEvent, void* pvParam )
{

  uint8_t* pucOffset = (uint8_t*)pvParam;

  if( nullptr == pucOffset )
    return pEvent;

  if( pucOffset[0] == pucOffset[1]++ )
    return pEvent;

  return nullptr;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 按时间从 旧向新 逐条处理每个满足过滤条件的记录
void* forEach_Forward( PROC_CacheItem pItemProc,
                        uint32_t       uFilter,
                        void*          pvParam )
{

  // 从最之前的开始找，
  uint32_t uEventPos,
           uEventCount = EventLogCache.Count;
  void*    pvRes = nullptr;

  if( 0 == uFilter )
    return nullptr;

  if( uEventCount >= SIZE_EVENT_CACHE )
    {
    uEventPos = EventLogCache.Position + 1; // 最老的一条记录位置
    if( uEventPos >= SIZE_EVENT_CACHE )
      uEventPos = 0;
    }
  else
    {
    uEventPos = 0;
    }

  // 搜索全部已存记录
  while( uEventCount-- > 0 )
    {
    // 取存贮空间指针
    struct tagEventLogCacheItem *pEvent = EventLogCache.Events + uEventPos;

    if( (pEvent->NewLogMarks & uFilter) == uFilter )
      {
      // 调用处理器
      pvRes = pItemProc( pEvent, pvParam );

      // 处理器有结果返回时，结束
      if( nullptr != pvRes )
        break;
      }

    // 卷回
    if( ++uEventPos >= SIZE_EVENT_CACHE )
      uEventPos = 0;
    }

  return pvRes;
}
//-----------------------------------------------------------------------------
// 按时间从 新向旧 逐条处理每个满足过滤条件的记录
void* forEach_Backward( PROC_CacheItem pItemProc,
                         uint32_t       uFilter,
                         void*          pvParam )
{

  // 从最之前的开始找，
  uint32_t uEventPos,
           uEventCount = EventLogCache.Count;
  void*    pvRes = nullptr;

  if( 0 == uFilter )
    return nullptr;

  uEventPos = EventLogCache.Position;  // 当前位置是最新的

  // 搜索全部已存记录
  while( uEventCount-- > 0 )
    {
    // 取存贮空间指针
    struct tagEventLogCacheItem *pEvent = EventLogCache.Events + uEventPos;

    if( (pEvent->NewLogMarks & uFilter) == uFilter )
      {
      // 调用处理器
      void* pvRes = pItemProc( pEvent, pvParam );

      // 处理器有结果返回时，结束
      if( nullptr != pvRes )
        break;
      }

    if( 0 == uEventPos )
      // 卷回
      uEventPos = SIZE_EVENT_CACHE - 1;
    else
      --uEventPos;
    }

  return pvRes;
}
//-----------------------------------------------------------------------------
// 读历史记录
//-----------------------------------------------------------------------------
//// 搜索特定记录的数量
////   在操作记录中搜索”遥控记录“
////   在告警记录中搜索”逻辑事件“
////   在故障记录中搜索”电流故障“，
//uint32_t getEventRecCount( TEventLogType uType )
//{

//  // 根据搜索的事件类型确定存贮类型
//  TEventLogType uSaveType;
//  switch( uType )
//    {
//    case mltCurProt:
//      uSaveType = mltFault;
//      break;
//    case mltRmtCtrl:
//      uSaveType = mltEvent;
//      break;
//    case mltLogic:
//      uSaveType = mltAlarm;
//      break;
//    default:
//      return 0;
//    }

//  // 读报告记录，统计数量
//  TEventLogItem eiLog;
//  uint32_t uIdx = 0, uCount = 0;
//  while( 0 == FIX_ReadEvtLogItem( uSaveType, uIdx++, &eiLog, ERD_FORWARD ) )
//    {
//    if( eiLog.Summary.State.Type == uType )
//      uCount++;
//    }

//  return uCount;
//}
//-----------------------------------------------------------------------------
//// 搜索特定记录的项
//// 在故障记录中搜索”电流故障“，或在操作记录中搜索”遥控记录“
//const TEventLogItem* getEventRecord( TEventLogType  uType,
//                                           uint32_t  uIndex,
//                                      TEventLogItem *pEvent,
//                                           uint32_t  uOption)
//{

//  // 根据搜索的事件类型确定存贮类型
//  TEventLogType uSaveType;
//  switch( uType )
//    {
//    case mltCurProt:
//      uSaveType = mltFault;
//      break;
//    case mltRmtCtrl:
//      uSaveType = mltEvent;
//      break;
//    case mltLogic:
//      uSaveType = mltAlarm;
//      break;
//    default:
//      return nullptr;
//    }

//  // 读报告记录，搜索指定的记录项
//  uint32_t uIdx = 0, uCount = 0;
//  TEventLogItem *pRes = nullptr;
//  while( 0 == FIX_ReadEvtLogItem( uSaveType, uIdx++, pEvent, uOption ) )
//    {
//    if( pEvent->Summary.State.Type == uType )
//      {
//      if( uCount == uIndex )
//        {
//        pRes = pEvent;
//        break;
//        }
//      else
//        uCount++;
//      }
//    }

//  return pRes;
//}
//=============================================================================
// 本地全局方法
//-----------------------------------------------------------------------------
// 初始化报告管理器
void EVTMGR_Init(void)
{

  memset( &EventLogCache, 0, sizeof(EventLogCache) );

  memset( &DispEventLogCache, 0, sizeof(DispEventLogCache) );

  SetNewEvents(0);
}
//-----------------------------------------------------------------------------
// 追加事件记录
// 追加的事件放到摘要缓冲区中
// 输入：
//   uRegNum：发生报告的寄存器
//   uAction：报告的行为 TRUE = 0->1;  FALSE = 1->0
void EVTMGR_AppendEvent( uint32_t uRegNum, uint32_t uAction )
{

  // 预判状态是否变位？
  if( REG_STATE == REG_TYPE(uRegNum) )
    {
//    // 取当前状态
//    TStateReg State = __GetStateReg_bk(uRegNum);
//    // 状态比较
//    if( (STATE_TRUE == State && STATE_TRUE == uAction) ||
//        (STATE_TRUE != State && STATE_TRUE != uAction) )
//      {
//      // 无变位则返回
//      return ;
//      }

//     // 事件的当前状态
//     if( _GetStateReg(uRegNum) )
//       _SetStateReg_bk(uRegNum, STATE_TRUE);
//     else
//       _SetStateReg_bk(uRegNum, STATE_FALSE);

#ifdef USE_GOOSE
    // 逻辑状态已变化，需要组织GOOSE信息量
    SetTodoTask( RTT_LOGIC_STATE );
#endif
     }

  // 取寄存器属性
  const TDevRegInfoItem* pRegInfo = DevIntf_GetRegInfo( uRegNum );
#ifdef USE_DEV_ASSERT
  // 没定义寄存器的TDevRegInfoItem，
  // 或 的TDevRegInfoItem中Property属性没有说事件类型
  DEV_ASSERT( nullptr == pRegInfo, GFC_ErrRegNum );
#endif

  // 该事件无存贮属性，直接返回
  //if( 0 == SIT_GetEvtType(pRegInfo->Property) )
  //  return ;

  // 取事件属性
  const TEventProperty* pEvtProp = DevIntf_GetEvtProp( pRegInfo->Event );

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pEvtProp, GFC_EmptyPtr );

  // TDevRegInfoItem中没填写Event项或非法相，或者 TEventProperty 中的数据不合法
  DEV_ASSERT( NUM_EVENT_DATA < pEvtProp->NbrOfData, // Wey.
              GFC_ErrParam );
#endif

  // 搜索可用的存贮位置
  uint32_t uNewPos;
  if( EventLogCache.Count > 0 )
    {
    if( EventLogCache.Position < SIZE_EVENT_CACHE - 1 )
      uNewPos = EventLogCache.Position + 1;
    else
      uNewPos = 0;
    }
  else
    uNewPos = 0;

  // 取存贮空间指针
  struct tagEventLogCacheItem *pEvent = EventLogCache.Events + uNewPos;
  memset( pEvent, 0, sizeof(*pEvent) );

  // 填写信息
  // 按 EDSFilter特性 确定事件是否需要弹窗或保存  Wey.2018.1.26
  if( STATE_TRUE == uAction )
    {
    // 0->1 事件
    if( 0 != (EDS_Save(pEvtProp->DSFilter) & EDS_SAVE01) )
      pEvent->NewLogMarks = NES_SAVE;
    else
      pEvent->NewLogMarks = 0;

    if( 0 != (pRegInfo->Property & SIT_EVT_Disp) &&
        0 != (pEvtProp->DSFilter & EDS_DISP01) )
      pEvent->NewLogMarks |= NES_DISP;
    }
  else
    {
    // 1->0 事件
    if( 0 != (pEvtProp->DSFilter & EDS_SAVE10) )
      pEvent->NewLogMarks = NES_SAVE;
    else
      pEvent->NewLogMarks = 0;

    if( 0 != (pRegInfo->Property & SIT_EVT_Disp) &&
        0 != (pEvtProp->DSFilter & EDS_DISP10) )
      pEvent->NewLogMarks |= NES_DISP;
    }

  //
  RTC_FillEventTime( &(pEvent->EventLog.Summary) );
  pEvent->EventLog.Summary.State.RegNum = uRegNum;
  pEvent->EventLog.Summary.State.Action = (STATE_TRUE == uAction)? EVENT_TRUE : EVENT_FALSE;
  pEvent->pRegInfo = pRegInfo;
  pEvent->pEvtProp = pEvtProp;

  if( 0 != (pRegInfo->Property & SIT_EVT_Comm) )    // 需要通信发送
    {
    pEvent->NewLogMarks |= NES_COMMs;
    SetNewEvents(NES_COMMs);
    }

  // 填写现场数据
  for( uint32_t uIdx = 0; uIdx < pEvtProp->NbrOfData; uIdx++ )
    {
    if( 0 != pEvtProp->FieldDataReg[uIdx] )
      pEvent->EventLog.FieldData[uIdx] = DevReg_Read( pEvtProp->FieldDataReg[uIdx] );
    }

  // 更新事件指针
  EventLogCache.Position = uNewPos;
  if( EventLogCache.Count < SIZE_EVENT_CACHE )
    EventLogCache.Count++;

  // 通知后台处理
  if( 0 != (pEvent->NewLogMarks & NES_SAVE) )
    SetTodoTask( RTT_SAVE_EVENT );

  // 若需要显示，则填写到事件显示缓冲区
  if( 0 != (pEvent->NewLogMarks & NES_DISP) )
    {
    if( DispEventLogCache.Count > 0 )
      {
      if( DispEventLogCache.Position < SIZE_EVENT_CACHE - 1 )
        uNewPos = DispEventLogCache.Position + 1;
      else
        uNewPos = 0;
      }
    else
      uNewPos = 0;

    struct tagEventLogCacheItem *pDispEvent = DispEventLogCache.Events + uNewPos;
    memcpy( pDispEvent, pEvent, sizeof(*pEvent) );

    // 更新显示事件指针
    DispEventLogCache.Position = uNewPos;
    if( DispEventLogCache.Count < SIZE_EVENT_CACHE )
      DispEventLogCache.Count++;
    }

#if WAVELOGGER_EN > 0
  // Wey. 2021.10.20 将触发录波交给事件处理器
  if( STATE_TRUE == uAction &&
      0 != (pRegInfo->Property & SIT_WAVELOG_TR) )    // 需要触发录波
    WAVELOG_Trigger( uRegNum );
#endif

  // Wey. 2021.10.20 将操作继电器交给事件处理器
  if( 0 != (pRegInfo->Property & SIT_RELAY_CTRL) )    // 需要操作继电器
    SetTodoTask ( RTT_RELAY_TRIP );
    
  if( DispEventLogCache.Count > 0 )   //wzj 修改只有保存时才检查弹出报告
    {
    SetTodoTask( RTT_DISP_EVENT );
    }
}
//-----------------------------------------------------------------------------
// 追加操作记录
// 输入：
//   uRegNum：发生报告的寄存器
//    uValue：操作的结果
// 返回：无
void EVTMGR_AppendOpLog( uint32_t uRegNum, uint32_t uValue )
{

  // 取寄存器属性
  const TDevRegInfoItem* pRegInfo = DevIntf_GetRegInfo( uRegNum );
#ifdef USE_DEV_ASSERT
  // 没定义寄存器的TDevRegInfoItem，  或 的TDevRegInfoItem中Property属性没有说事件类型
  DEV_ASSERT( nullptr == pRegInfo, GFC_ErrRegNum );
#endif

  // 搜索可用的存贮位置
  uint32_t uNewPos;
  if( EventLogCache.Count > 0 )
    {
    if( EventLogCache.Position < SIZE_EVENT_CACHE - 1 )
      uNewPos = EventLogCache.Position + 1;
    else
      uNewPos = 0;
    }
  else
    uNewPos = 0;

  // 取存贮空间指针
  struct tagEventLogCacheItem *pEvent = EventLogCache.Events + uNewPos;
  memset( pEvent, 0, sizeof(*pEvent) );

  pEvent->NewLogMarks = NES_COMMs;
  RTC_FillEventTime( &(pEvent->EventLog.Summary) );
  if( REG_SWITCH == REG_TYPE(uRegNum) )
    pEvent->EventLog.Summary.State.Action = uValue? EVENT_TRUE : EVENT_FALSE;
  else
    pEvent->EventLog.Summary.State.Action = EVENT_TRUE;
  pEvent->pRegInfo = pRegInfo;
  pEvent->pEvtProp = nullptr;

  pEvent->EventLog.Summary.State.RegNum = uRegNum;
  pEvent->EventLog.FieldData[0] = uValue;

  // 更新事件指针
  EventLogCache.Position = uNewPos;
  if( EventLogCache.Count < SIZE_EVENT_CACHE )
    EventLogCache.Count++;

  //
  SetNewEvents(NES_COMMs);
}
//-----------------------------------------------------------------------------
// 保存事件记录
void EVTMGR_SaveEvent(void)
{

  forEach_Forward( saveNewEvent, NES_SAVE, 0 );

  // 清标志
  ClrTodoTask( RTT_SAVE_EVENT );

//  if( DispEventLogCache.Count > 0 )
//    {
//    SetTodoTask( RTT_DISP_EVENT );
//    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 统计指定条件的新记录数量
// 输入：
//   uFilter: 过滤条件，NES_xxx，用于各任务只搜索自己需要的事件
// 返回：
//   该过滤条件下可用的新事件数量
uint32_t EVTMGR_GetNewEventCount( uint32_t uFilter )
{

  if( 0 != (uFilter & NES_COMMs) &&
      0 == GetNewEventSign( uFilter ) )
    return 0;

  uint32_t uCount = 0;

  forEach_Forward( countEvents, uFilter, &uCount );

  if( 0 != (uFilter & NES_COMMs) && 0 == uCount )
    ClrNewEventSign( uFilter );

  return uCount;
}
//-----------------------------------------------------------------------------
// 取指定类型的记录数量
// 输入：
//   uType:  事件类型，mltNew只查NES_DISP, 其它调用EVTMGR_GetNewEventCount
// 返回：
//   该过滤条件下可用的新事件数量
uint32_t EVTMGR_GetEventCount( TEventLogType uType )
{

  uint32_t uCount;
  switch( uType )
    {
    case mltNew:  // 只用于显示
      //uCount = EVTMGR_GetNewEventCount( NES_DISP );
      uCount = DispEventLogCache.Count;
      break;

    case mltEvent:
      uCount = LogIndexer.EventLogs.Count;
      break;

    case mltAlarm:
      uCount = LogIndexer.AlarmLogs.Count;
      break;

    case mltFault:
      uCount = LogIndexer.FaultLogs.Count;
      break;

    default:
//      uCount = getEventRecCount( uType );
      uCount = 0;
      break;
    }

  return uCount;
}
//-----------------------------------------------------------------------------
// 取指定类型的记录项
// 输入：
//    uType:  事件类型
//   uIndex： 事件序号
//   pEvent:  存贮事件空间
// 返回：
//   null=读取失败，others=读取成功
const TEventLogItem* EVTMGR_GetEventItem( TEventLogType  uType,
                                               uint32_t  uIndex,
                                          TEventLogItem *pEvent )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pEvent, GFC_EmptyPtr );
#endif

  const TEventLogItem* pRes;
  switch( uType )
    {
    case mltNew:  // 只用于显示
      {
//      pRes = EVTMGR_Search( NES_DISP, uIndex, ERD_BACKWARD );
//      memcpy( pEvent, pRes, sizeof(TEventLogItem) );
      if( uIndex >= DispEventLogCache.Count )
        pRes = nullptr;
      else
        {
        uint32_t uPos;
        if( uIndex <= DispEventLogCache.Position )
          uPos = DispEventLogCache.Position - uIndex;
        else
          {
          uPos = uIndex - DispEventLogCache.Position;
          uPos = SIZE_EVENT_CACHE - uPos;
          }

        pRes = &DispEventLogCache.Events[uPos].EventLog;
        memcpy( pEvent, pRes, sizeof(TEventLogItem) );
        }
      break;
      }

    case mltEvent:
    case mltAlarm:
    case mltFault:
      {
      if( 0u == FIX_ReadEvtLogItem( uType, uIndex, pEvent, ERD_BACKWARD ) )
        pRes = pEvent;
      else
        pRes = nullptr;
      break;
      }

    default:
//      pRes = getEventRecord( uType, uIndex, pEvent, ERD_BACKWARD );
      pRes = nullptr;
      break;
    }

  return pRes;
}
//-----------------------------------------------------------------------------
// 读取事件描述
uint32_t EVTMGR_GetEventDesp( TEventWithProperty *pEvent )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pEvent, GFC_EmptyPtr );
#endif

  if( nullptr == pEvent->RegInfo || nullptr == pEvent->EvtProp )
    {
    // 寄存器的序号恢复寄存器地址
    uint32_t uRegNum = pEvent->EvtLog.Summary.State.RegNum;
    pEvent->RegInfo = DevIntf_GetRegInfo( uRegNum );
    if( nullptr == pEvent->RegInfo )
      return 2;

    pEvent->EvtProp = DevIntf_GetEvtProp( pEvent->RegInfo->Event );
    if( nullptr == pEvent->EvtProp )
      return 3;

    // 复制动作名称
//    strcpy( pEvent->EvtDesp,
//             ((EVENT_TRUE == pEvent->EvtLog.Summary.State.Action)?
//                             pEvent->EvtProp->OnName :
//                             pEvent->EvtProp->OffName)
//          );

    // 允许无动作名称
    const char* pcActName;
    if( EVENT_TRUE == pEvent->EvtLog.Summary.State.Action )
      pcActName = GetMultiLangString(pEvent->EvtProp->OnNameId);
    else
      pcActName = GetMultiLangString(pEvent->EvtProp->OffNameId);

    if( nullptr != pcActName && 0 != pcActName[0] )
      {
#ifndef __vmSIMULATOR__
      strncpy( pEvent->EvtDesp, pcActName, sizeof(pEvent->EvtDesp) - 1 );
#else
      // 使用安全拷贝方法
      strncpy_s( pEvent->EvtDesp, sizeof(pEvent->EvtDesp), 
                 pcActName,       sizeof(pEvent->EvtDesp) - 1);
#endif
      
      pEvent->EvtDesp[sizeof(pEvent->EvtDesp) - 1] = 0;
      }
    else
      pEvent->EvtDesp[0] = 0;
    }

  return 0;
}
//-----------------------------------------------------------------------------
// 清全部新事件标志
// 输入：
//   uFilter: 过滤条件，NES_xxx，用于各任务只搜索自己需要的事件
// 返回： 无
void EVTMGR_ClearNewEventMarks( uint32_t uFilter )
{

  if( 0 != (uFilter & NES_DISP) )
    {
    DispEventLogCache.Count = 0;
    DispEventLogCache.Position = 0;
    }
  else
    forEach_Forward( clearMark, uFilter,
#ifndef __QT__
                     (void*)uFilter );
#else
                     (void*)(uint64_t)uFilter );
#endif
}
//-----------------------------------------------------------------------------
// 按 时间 搜索新记录
// 输入：
// 输入：
//    uFilter: 过滤条件，NES_xxx，用于各任务只搜索自己需要的事件
//    uOffset: 从指定方向找第几条有效的记录  0~ SIZE_EVENT_CACHE-1
//   uOptions: 指定搜索任务的工作方式  ERD_xxx宏
//             ERD_FORWARD：  向前 即从缓冲区中时间最老的记录开始
//             ERD_BACKWARD： 向后 即从缓冲区中时间最新的记录开始
//             ERD_CLEARMARK：设置=清除(将来取不到了) 不置=清除(将来还可以再取)
// 返回：
//   搜索到的事件记录
const TEventLogItem* EVTMGR_Search( uint32_t uFilter,
                                     uint32_t uOffset,
                                     uint32_t uOptions )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( uOffset >= SIZE_EVENT_CACHE, GFC_EmptyPtr );
#endif

  uint8_t  pucOffset[2];

  pucOffset[0] = uOffset;
  pucOffset[1] = 0;

  // 按 iOrder 选择搜索方向
  void  *pvRes;
  if( ERD_BACKWARD == (uOptions & ERD_BACKWARD) )
    pvRes = forEach_Backward( fetchEvent, uFilter, pucOffset );
  else
    pvRes = forEach_Forward ( fetchEvent, uFilter, pucOffset );

  if( nullptr == pvRes )
    return nullptr;

  struct tagEventLogCacheItem *pItem = (struct tagEventLogCacheItem*)pvRes;

  if( uOptions & ERD_CLEARMARK )
    pItem->NewLogMarks &= ~uFilter;

  return &(pItem->EventLog);
}
//-----------------------------------------------------------------------------
// 取走新记录
// 按 旧到新的时间顺序， 记录取走后该标志被清除
// 输入：
//   uFilter: 过滤条件，NES_xxx，用于各任务只搜索自己需要的事件
// 返回：
//   搜索到的事件记录
const TEventLogItem* EVTMGR_Fetch( uint32_t uFilter )
{

  void *pvRes = forEach_Forward( fetchEvent, uFilter, nullptr );
  if( nullptr == pvRes )
    return nullptr;

  struct tagEventLogCacheItem *pItem = (struct tagEventLogCacheItem*)pvRes;
  pItem->NewLogMarks &= ~uFilter;

  return &(pItem->EventLog);
}
//-----------------------------------------------------------------------------
