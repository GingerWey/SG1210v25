//-----------------------------------------------------------------------------
/*
 File        : WaveLogger.c
 Version     : V1.10
 By          : 银网科技

 Description :定义录波器接口

 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "WaveLogger.h"

#include "NvRAM.h"
#include "DevEvtMgr.h"

#ifdef __SIMULATOR__
 extern void RTC_FillEventTime( struct tagEventLogSummary *pEvtSum );
#else
  #include "rtc.h"
  #include "gpio.h"
  #include "iwdg.h"
  #include <cmsis_os.h>
#endif

#include "DevRegs.h"
#include "DevDebug.h"

#include "DevClock.h"

#include <string.h>
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0 && NUM_WAVERECORDER > 0

// 本录波器用于GPT30，只支持1组录波器
// 1.m_ucRceNo用作录波工作使能
// 2.波形录满后，进入保存过程（Save）；
// 3.保存完成后，自动启动预录过程。
#if NUM_WAVERECORDER > 1
  #error 本录波器用于GPT30，只支持1组录波器，将m_ucRceNo用作录波工作使能
#endif
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 每条录波占用的Flash扇区数量
#define NUM_Sectors_WaveLog  ((sizeof( TWaveLogItem ) + (SIZE_SECTOR - 1)) / SIZE_SECTOR) 
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 取录波记录的起始地址
uint32_t __WavelogAddress(uint32_t uIndex)
{

  uint32_t uPosition;
  uPosition = LogIndexer.WaveLogs.Position;  // 当前位置是最新的
  
  if( uPosition >= uIndex )
    uPosition -= uIndex;
  else
    uPosition = LogIndexer.WaveLogs.Count - (uIndex - uPosition);
  
  // 起始地址
  uint32_t szSector = NvRAM_FlashSectorSize(),
           uAddress = uPosition * 
                     (((sizeof(TWaveLogItem) + szSector - 1) / szSector) * szSector);

  return uAddress;
}
//=============================================================================
// 本地对象
//-----------------------------------------------------------------------------
// 波形记录器
class TWaveLogger
{
private:
  volatile uint8_t   m_ucRceNo;            // 记录器序号 0/1
  volatile uint8_t   m_ucActived;          // 记录器是否激活
  volatile uint32_t  m_uPosition;          // 当前记录器的存贮位置 
  volatile uint32_t  m_uCount;             // 当前记录器中数据数量
  volatile uint32_t  m_uPreRecCnt;         // 预录波采样点数

  volatile uint32_t  m_uTrigPos[NUM_WAVERECORDER];   // 触发点
  TWaveLogItem       m_Recorders[NUM_WAVERECORDER];   // 录波器

public:
  TWaveLogger()  {}
  
  void Reset(void);
    
  void Stop (void)  { m_ucRceNo = NUM_WAVERECORDER; }
  void Start(void)  
    { 
      Reset();  
      m_ucRceNo = 0; 
      memset( m_Recorders + m_ucRceNo, 0, sizeof(TEventLogSummary) );
    }

  TWaveLogItem* GetRecorder(void) { return m_Recorders; }

  void Record(void);

  void Trigger(uint32_t uRegNum);
  
  void Save(void);

protected:
  void DoRecord( TWaveLogItem *pRecorder );

  bool WriteLogToFlash(uint32_t  uPosition, uint32_t uRcdrNo);
  bool WriteToFlash   (uint32_t  uAddress,
                           void *pvBuf,
                       uint32_t  uNumBytes,
                           bool  bEarse = false );
};
//----------------------------------------------------------------------------
// 复位位录波器
void TWaveLogger::Reset(void)
{

  // GPT30专用，
  m_ucRceNo = NUM_WAVERECORDER;  // NUM_WAVERECORDER：停止
  m_ucActived  = 0;
  m_uPosition = 0;
  m_uCount    = 0;

  // 计算预录波采样点数
  m_uPreRecCnt = _GetDevCfgReg(REG_WAVELOG_PRE);
  if( m_uPreRecCnt <= 1 )
    m_uPreRecCnt = NUM_WAVELOG_PRERECODE;
  else if( m_uPreRecCnt > NUM_WAVELOG_CYCLES / 2 )
    m_uPreRecCnt = NUM_WAVELOG_CYCLES / 2;
  
  m_uPreRecCnt *= NUM_SAMPLES_PER_PEROID;
   
  memset( (void*)m_uTrigPos, 0, sizeof(m_uTrigPos) );
  memset( &m_Recorders, 0, sizeof(m_Recorders) );
}    
//-----------------------------------------------------------------------------
// 记录数据断面
void TWaveLogger::Record(void)
{
  
  // GPT30专用，
  // m_ucRceNo用于决定是否录波
  if( m_ucRceNo < NUM_WAVERECORDER )
    DoRecord( m_Recorders + m_ucRceNo );
}  
//-----------------------------------------------------------------------------
void TWaveLogger::Trigger(uint32_t uRegNum)
{ 

  // 正录波？则返回
  if( 0 != m_ucActived )
    return ;
  
  // 标记【正在录波】
  m_ucActived = 0xAA;
  TWaveLogItem* pRecorder = &m_Recorders[m_ucRceNo];
  
//  RTC_FillEventTime( &(pRecorder->Summary) );
  DEVCLK_FillEventSummary( &DevClock, &(pRecorder->Summary) );
  pRecorder->Summary.State.RegNum = uRegNum;
  pRecorder->Summary.State.Action = EVENT_TRUE;
  pRecorder->Summary.State.Type   = NUM_WAVELOG_PRERECODE; //_GetDevCfgReg(REG_WAVELOG_PRE);
}
//-----------------------------------------------------------------------------
// 保存录波到Flash
void TWaveLogger::Save(void)
{
  
  TRingStore *pIndexer = &LogIndexer.WaveLogs;
  
  // 计算记录项在录小组环形索引区中的位置
  uint32_t uPosition;
  if( pIndexer->Count > 0 )
    {
    if( pIndexer->Position < pIndexer->Total - 1 )  // Wey. 2018.8.31
      uPosition = pIndexer->Position + 1;        
    else
      uPosition = 0;
    }
  else
    uPosition = 0;

  // GPT30专用，
  // m_ucRceNo用于决定是否录波
  // uint32_t uRcdrIdx = (0 == m_ucRceNo)? 1 : 0;
  uint32_t uRcdrIdx = 0;
  // 保存到Flash
  if( true == WriteLogToFlash( uPosition, uRcdrIdx ) )
    {
    // Update Position
    LogIndexer.WaveLogs.Position = uPosition;
    if( LogIndexer.WaveLogs.Total > LogIndexer.WaveLogs.Count )
      LogIndexer.WaveLogs.Count++;
    
    TWaveLogItem* pRecorder = &m_Recorders[uRcdrIdx];

    // 保存录波索引 
    FIX_SaveLogIndexer();   // GPT30
//    if( 0 == FIX_SaveLogIndexer() )
//      // 标记已保存
//      pRecorder->Summary.Time.Year = 0;

//    // 若是手动触发，则通知任务弹窗体
//    if( REG_EO_WAVELOG_TRIGGER == pRecorder->Summary.State.RegNum )
//      SetGUIState( RGS_WAVLOGFORM );
    } 
}
//-----------------------------------------------------------------------------
// 记录数据断面
void TWaveLogger::DoRecord(TWaveLogItem *pRecorder)
{
    
  // 计算存贮位置
  uint32_t uPosition;
  if( 0 < m_uCount )
    {
    uPosition = m_uPosition + 1;
    
    if( NUM_SAMPLOG_PER_CHL <= uPosition )
      uPosition = 0;
    }
  else
    uPosition = 0;
  
  // AD采样
  for( uint32_t uIdx = 0; uIdx < NUM_ADC_CHANNELS; uIdx++ )
    pRecorder->Measure[uIdx][uPosition] = DevCache.ADCCrsSection[uIdx];

  // 
  uint32_t u8Pos = uPosition >> 3,
           uMask = (1 << (uPosition & 0x7));    
  // 
  if( 1 == uMask )
    {
    for( uint32_t uIdx = 0; uIdx < NUM_WAVELOG_BINCHLS; uIdx++ )
      pRecorder->Binary[uIdx][u8Pos] = 0;
    }

#ifdef YXs_State
  // 开入状态
  uint32_t uYX = YXs_State(0);
  for( uint32_t uIdx = 0; uIdx < NUM_DigInputs; uIdx++ )
    if( 0 != (uYX & (1 << uIdx)) )
      pRecorder->Binary[uIdx][u8Pos] |= uMask;
    else
      pRecorder->Binary[uIdx][u8Pos] &= ~uMask;
#endif

#ifdef TRIP1_State
  // 开出状态
   if( 0 != TRIP1_State )
    pRecorder->Binary[NUM_DigInputs + 0][u8Pos] |= uMask;
  else
    pRecorder->Binary[NUM_DigInputs + 1][u8Pos] &= ~uMask;

   if( 0 != TRIP2_State )
    pRecorder->Binary[NUM_DigInputs + 1][u8Pos] |= uMask;
  else
    pRecorder->Binary[NUM_DigInputs + 1][u8Pos] &= ~uMask;
#endif

  // 更新指针
  if( 0 == m_ucActived )
    {
    if( m_uPreRecCnt > m_uCount )
      m_uCount++;

    m_uPosition = uPosition;
    }    
  else if( NUM_SAMPLOG_PER_CHL > m_uCount )
    {
    m_uCount++;

    m_uPosition = uPosition;
    }
  else
    {
    m_uTrigPos[m_ucRceNo] = uPosition;

    // 本记录器录满

    // GPT30专用
    // m_ucRceNo=NUM_WAVERECORDER用于暂停录波
    m_ucRceNo = NUM_WAVERECORDER;
    m_ucActived = 0;
    m_uPosition = 0;
    m_uCount    = 0;

    // GPT30只有一组录波器
    //memset( m_Recorders + m_ucRceNo, 0, sizeof(TEventLogSummary) );

    SetTodoTask( RTT_SAVE_WAVLOG );
    }
}
//-----------------------------------------------------------------------------
bool TWaveLogger::WriteToFlash( uint32_t  uAddress,
                                    void *pvBuf,
                                uint32_t  uNumBytes,
                                    bool  bEarse )
{
  
  uint32_t uWritten = NvRAM_WriteToFlash( TOKEN_NvRAM_ACCESS,
                                          uAddress,
                                          pvBuf,
                                          uNumBytes,
                                          bEarse,
  
#ifndef __SIMULATOR__
                                          IWDG_FeedDog
#else
                                          NULL
#endif
                                        );

  return bool(uWritten == uNumBytes);
}
//-----------------------------------------------------------------------------
// 保存录波记录到Flash
bool TWaveLogger::WriteLogToFlash(uint32_t uPosition, uint32_t uRcdrNo )
{

  // 起始地址
  uint32_t szSector = NvRAM_FlashSectorSize(),
           uAddress = uPosition * 
                      (((sizeof(TWaveLogItem) + szSector - 1) / szSector) * szSector);

  TWaveLogItem* pRecorder = &m_Recorders[uRcdrNo];

  // -------------- 录波数据头 ------------------
  // 录波数据头是否有效？
  if( pRecorder->Summary.Time.Year < 2000 )
    return false;

  bool bRes = WriteToFlash( uAddress, pRecorder, sizeof(TWaveLogItem) );
  
//  // 录波数据头的长度
//  bool bRes = false;
//  uint32_t uSize = (uint8_t*)(pRecorder->Measure) - 
//                   (uint8_t*)&(pRecorder->Summary);

//  // 写录波数据头
//  if( false == WriteToFlash( uAddress, &pRecorder->Summary, uSize, true ) )
//    return false;

//  // 后续写入的地址
//  uAddress += uSize;

//  // -------------- 模拟量 ------------------
//  // 由模拟量的触发位置计算本次录波的启始点
//  uint32_t uBufPos = m_uTrigPos[uRcdrNo] + 1;
//  if( NUM_SAMPLOG_PER_CHL <= uBufPos )
//    uBufPos = 0;

//  // 首次要写入的位置
//  uSize = (SIZE_SAMPLOG_ADCCHL - uBufPos) * sizeof(pRecorder->Measure[0][0]);

//  // 模拟通道逐个写入
//  for( uint32_t uIdx = 0; uIdx < NUM_ADC_CHANNELS; uIdx++ )
//    {
//    // 写前半段
//    if( false == WriteToFlash( uAddress, 
//                              &(pRecorder->Measure[uIdx][uBufPos]),
//                               uSize ) )
//      return false;

//    if( uBufPos > 0 )
//      {
//      // 写卷回的后半段
//      if( false == WriteToFlash( uAddress + uSize, 
//                                 pRecorder->Measure[uIdx],
//                                 sizeof(pRecorder->Measure[0]) - uSize) )
//        return false;
//      }

//    uAddress += sizeof(pRecorder->Measure[0]);
//    }

//  // -------------- 数字量 ------------------
//  uBufPos = m_uTrigPos[uRcdrNo] + 1;
//  if( NUM_SAMPLOG_PER_CHL <= uBufPos )
//    uBufPos = 0;

//  // 先写入的字节数量
//  uSize = (NUM_SAMPLOG_PER_CHL - uBufPos + 7) / 8;

//  uBufPos /= 8;  // 对齐到字节

//  // 数字通道逐个写入
//  for( uint32_t uIdx = 0; uIdx < NUM_WAVELOG_BINCHLS; uIdx++ )
//    {
//    // 写前半段
//    if( false == WriteToFlash( uAddress, 
//                              &(pRecorder->Binary[uIdx][uBufPos]), 
//                               uSize) )
//      return false;
//    
//    if( uBufPos > 0 )
//      {
//      // 写卷回的后半段
//      if( false == WriteToFlash( uAddress + uSize, 
//                                 pRecorder->Binary[uIdx],
//                                 sizeof(pRecorder->Binary[0]) - uSize ) )
//        return false;                                   
//      }
//    
//    uAddress += sizeof(pRecorder->Binary[0]);
//    }

//  // debug
//  uAddress = uPosition * 
//             (((sizeof(TWaveLogItem) + szSector - 1) / szSector) * szSector);

//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;
//  NvRAM_FlashRead( uAddress, pRecorder->Measure, szSector );  uAddress += szSector;

  return bRes;
}
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 录波器
TWaveLogger WaveLogger;
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void WAVELOG_Init(void)
{
  
  WaveLogger.Reset(); 
  
  NvRAM_InitFlash();
}
//-----------------------------------------------------------------------------
// 复位录波器
void WAVELOG_Reset(void)
{

  WaveLogger.Reset(); 
}
//-----------------------------------------------------------------------------
// 停止录波
void WAVELOG_Stop(void)
{
  
  WaveLogger.Stop();
}
//-----------------------------------------------------------------------------
// 启动录波器
void WAVELOG_Start(void)
{

  WaveLogger.Start();
}
//-----------------------------------------------------------------------------
// 记录
void WAVELOG_Record(void)
{
  
  WaveLogger.Record(); 
}
//-----------------------------------------------------------------------------
// 激活
void WAVELOG_Trigger(uint32_t uRegNum)
{
  
  WaveLogger.Trigger( uRegNum ); 
}
//-----------------------------------------------------------------------------
// 保存
void WAVELOG_Save(void)
{
  
//  uint32_t uStart = HAL_GetTick();
  
  // 保存到Flash
  // SST25VF016: AAI写 517ms
  // SST25VF064:  页写    ms
  WaveLogger.Save();  

  ClrTodoTask(RTT_SAVE_WAVLOG);

  SetTodoTask(RTT_SHOW_WAVLOG);

//  uStart = HAL_GetTick() - uStart;

//  _SetCommonReg( REG_DEV_Probe6, uStart );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 获取总记录数量
uint32_t WAVLOG_GetLogCount(void)
{
  
  return LogIndexer.WaveLogs.Count;  
}
//-----------------------------------------------------------------------------
// 读录波记录摘要
// 新向旧检索
uint32_t WAVLOG_GetLogSummary(TEventLogSummary* pSummary, uint32_t uIndex)
{
  
#ifdef USE_DEV_ASSERT
  // 指针为空
  DEV_ASSERT( 0 == pSummary, GFC_EmptyPtr );
    
  // 没定义寄存器的TDevRegInfoItem
  DEV_ASSERT( uIndex >= LogIndexer.WaveLogs.Count, GFC_ErrParam );
#endif
    
  uint32_t uAddr  = __WavelogAddress(uIndex),
           uBytes = NvRAM_FlashRead( uAddr, pSummary, sizeof(TEventLogSummary) );

  if( sizeof(TEventLogSummary) != uBytes )
    return 3;
    
  return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 取录波器
TWaveLogItem* WAVLOG_GetRecorder(void)
{
  
  return WaveLogger.GetRecorder();
}
//-----------------------------------------------------------------------------
// 读模拟通道数据
uint32_t WAVLOG_ReadADChl(uint32_t uAddress,
                          uint32_t uChlIdx,
                          uint32_t uPosition,
                          void*    pBuff,
                          uint32_t uSampsToRead)
{

  if( NUM_SAMPLOG_PER_CHL <= uPosition  ||
      NUM_ADC_CHANNELS <= uChlIdx )
    return 0;
  
  constexpr auto bytesOfADCTrack = SIZE_SAMPLOG_ADCCHL * sizeof(uint16_t);

  auto uBytePosition = uPosition * sizeof(uint16_t), // 
       uBytesToRead  = uSampsToRead * sizeof(uint16_t);
  if( bytesOfADCTrack < uBytePosition + uBytesToRead )
    uBytesToRead =  bytesOfADCTrack- uBytePosition;

  uAddress += (uChlIdx * bytesOfADCTrack) + uBytePosition;
 
  return NvRAM_FlashRead( uAddress, pBuff, uBytesToRead );
}
//-----------------------------------------------------------------------------
// 读数字通道数据
uint32_t WAVLOG_ReadBinChl(uint32_t  uAddress,
                           uint32_t  uChlIdx,
                           uint32_t  uPosition,
                           void     *pBuff,
                           uint32_t  uSampsToRead)
{

  if( NUM_SAMPLOG_PER_CHL <= uPosition  ||
      NUM_WAVELOG_BINCHLS <= uChlIdx )
    return 0;
  
  constexpr auto bytesOfADCTrack = SIZE_SAMPLOG_ADCCHL * sizeof(uint16_t);
  constexpr auto beginOfBINTrack = bytesOfADCTrack * NUM_ADC_CHANNELS;
  constexpr auto bytesOfBINTrack = SIZE_WAVELOG_BINCHL;

  uint32_t uBytePosition = (uPosition >> 3),  // Byte对齐
           uBytesToRead  = (uSampsToRead + 7) >> 3;
  if( bytesOfBINTrack < (uBytePosition + uBytesToRead) )
    uBytesToRead = bytesOfBINTrack - uBytePosition;

  uAddress += beginOfBINTrack +
              (uChlIdx * bytesOfBINTrack) + uBytePosition;

  return NvRAM_FlashRead( uAddress, pBuff, uBytesToRead );
}
//-----------------------------------------------------------------------------
// 读录波通道数据
// 读数字通道时，要求uPosition 8位对齐
uint32_t WAVLOG_ReadChlData( uint32_t uIndex,              // 录波记录序号，ZOrder
                             uint32_t uChlIdx,             // 通道号
                             uint32_t uPosition,           // 位置, 样本序列
                             void*    pBuff,               // 数据区
                             uint32_t uSampsToRead )       // 采样点数
{
  
  if( 0 == uSampsToRead || nullptr == pBuff )
    return 0;
  
  uint32_t uRes, uAddress = __WavelogAddress(uIndex);
  
  const TWaveLogItem *pRecorder = WaveLogger.GetRecorder();
  uAddress += (uint8_t*)(pRecorder->Measure) - (uint8_t*)&(pRecorder->Summary);
  
  if( uChlIdx < NUM_ADC_CHANNELS )
    uRes = WAVLOG_ReadADChl( uAddress, 
                             uChlIdx, 
                             uPosition, 
                             pBuff, 
                             uSampsToRead );
  else if( uChlIdx < NUM_ADC_CHANNELS + NUM_WAVELOG_BINCHLS )
    uRes = WAVLOG_ReadBinChl( uAddress,
                              uChlIdx - NUM_ADC_CHANNELS,
                              uPosition,
                              pBuff,
                              uSampsToRead );
  else
    uRes = 0;

  return uRes;
}
//-----------------------------------------------------------------------------
// 读全部通道
uint32_t WAVELOG_ReadWavelog( uint32_t uIndex,  void* pBuff )
{
  
  uint32_t uAddress = __WavelogAddress(uIndex);
  
  const TWaveLogItem *pRecorder = WaveLogger.GetRecorder();
  uAddress += (uint8_t*)(pRecorder->Measure) - (uint8_t*)&(pRecorder->Summary);
  
  uint32_t uBytesToRead  = sizeof( pRecorder->Measure ) + 
                           sizeof( pRecorder->Binary ),
           uSizeofSector = NvRAM_FlashSectorSize(),
           uNumSectors   = uBytesToRead / uSizeofSector,
           uBytesRead    = 0;

  uint8_t *pucBuf = (uint8_t*)pBuff;                           
  for( uint32_t uIdx = 0; uIdx < uNumSectors; uIdx++ )
    {
    uBytesRead += NvRAM_FlashRead( uAddress, pucBuf, uSizeofSector );  
    
    pucBuf   += uSizeofSector;
    uAddress += uSizeofSector;      
    }
    
  if( uBytesRead < uBytesToRead )
    uBytesRead += NvRAM_FlashRead( uAddress, pucBuf, uBytesToRead - uBytesRead ); 
  
  return uBytesRead;
}
//-----------------------------------------------------------------------------
#endif // WAVELOGGER_EN > 0 && NUM_WAVERECORDER > 0
//-----------------------------------------------------------------------------

