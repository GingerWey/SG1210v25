//-----------------------------------------------------------------------------
/*
 File        : InspSvr.h
 Version     : V1.10
 By          : 银网科技

 Description :Inspector服务器协议实现

 Date       : 2023.12.05
 
 Inspector服务用于测试工具对完成装置内部信息检测
 
 扩展Wavelog读取功能
*/
//-----------------------------------------------------------------------------
#include "InspSvr.h"

#include "iwdg.h"

#include "AES128.h"
#include "RNGen.h"
#include "CRC1632.h"
#include "RamHeap.h"

#include "DevClock.h"
#include "WaveLogger.h"

#include "DevRegs.h"
#include "GPVersion.h"

#include "SocketBase.h"

#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define SIZE_RX_BUF    1460
#define SIZE_TX_BUF    1460
//=============================================================================
// 报文帧定义
//-----------------------------------------------------------------------------
// 帧结构
//      Head     LenL     LenH      Head     Func         CRC16L  CR16CH
//     ------  -------- --------  --------  ------  ....  ------  ------
//       E6H                         E6H       
//   Len   = Head ->  CRCH
//   CRC16 = Head ->  CRCL - 1
//
//-----------------------------------------------------------------------------
#define SKEY_ISP            "[9rfW*e^y.0L1|0]"
//-----------------------------------------------------------------------------
// 同步头
#define ISP_HEAD             0xE6
//-----------------------------------------------------------------------------
// 功能码
#define ISP_RESET            0x11    // 通信复位    
                                     // 下行：要求装置复位  上行：ISP_DEVINFO
                                  
#define ISP_DEVINFO          0x12    // 读装置信息  
                                     // 下行： 无
#define ISP_DEVRTRES         0x14    // 读装置资源                                
                                  
#define ISP_REGREAD          0x16    // 读寄存器值
                                     // 下行：请求一组寄存器 上行：ISP_REGREAD
                                  
#define ISP_REGPROP          0x17    // 读寄存器属性  
                                     // 下行：请求一个寄存器的信息 上行：ISP_REGPROP
                                  
#define ISP_REGWRITE         0x19    // 写寄存器
                                      // 下行：写连续的一组寄存器 上行：ISP_REGWRITE
#define ISP_REGWRTEXEC       0x1A    // 确定刚下发的寄存器写操作
                                     // 下行：请求一组寄存器 上行：ISP_REGWRTEXEC
                                     
#define ISP_SETAOS           0x1C    // 配置自动发送组
                                     // 下行：请求一组寄存器 上行：ISP_SETAOS
#define ISP_CLRAOS           0x1D    // 清自动发送组
#define ISP_AOSDATA          0x1E    // AOS数据包   上行：ISP_AOSDATA
 
// 读Wavelog 
#define ISP_WL_DIGEST        0x30    // 读取录波器描述
#define ISP_WL_CHLINFO       0x31    // 读取录波通道描述
#define ISP_WL_RECINFO       0x32    // 读录波记录描述
#define ISP_WL_RDDATA        0x33    // 读通道数据
#define ISP_WL_CLEAR         0x3C    // 清录波记录
  #define WL_CLEAR_TOKEN     0xCEA00CEA
//=============================================================================
// 局部数据结构
//-----------------------------------------------------------------------------
typedef struct __PACKED_BEG tagISPMsgeHeader
{
  uint8_t   ucSync1;
  uint16_t  uwLength;
  uint8_t   ucSync2;

  uint8_t   ucFunc;

  uint8_t   pData[1500];
} __PACKED_END TISPMsgeHeader;
//-----------------------------------------------------------------------------
typedef struct __PACKED_BEG tagWaveLogChannelDesp
{
  uint8_t     ucChlType;        // 0=ADC 1=DIG
  uint8_t     ucChlIndex;       // 通道序号，按录波器存贮排序

  uint16_t    uwSampleZero,     // 零值的采样值
              uwSampleMax;      // 采样的正向最大值

  uint32_t    uPeakValue;       // 实际值的峰值
  
  const char  Name[10];
  const char  DimName[6];
} __PACKED_END GWaveLogTrackDesp;
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
// 装置参数区的编辑区
extern TDeviceFixed DevCfgForEdit;
//-----------------------------------------------------------------------------
// 录波通道数据
const GWaveLogTrackDesp FWLChlsDesp[] =
{  
  { 0, 0, 2048, 2048, 390, "电压", "V" }  
 ,{ 0, 1, 2048, 2048, 141, "电流", "A" }  
 ,{ 1, 0,    0,    1,   1, "引弧", ""  }  
 ,{ 1, 1,    0,    1,   1, "吹弧", ""  }  
 ,{ 1, 2,    0,    1,   1, "合位", ""  }  
 ,{ 1, 3,    0,    1,   1, "分位", ""  }  
 ,{ 1, 4,    0,    1,   1, "合闸", ""  }  
 ,{ 1, 5,    0,    1,   1, "跳闸", ""  } 
};
#define NUM_FWLTracksDesp   (sizeof(FWLChlsDesp) / sizeof(FWLChlsDesp[0]))
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
#if AOS_ENABLE > 0
 TInspectorAOS AOSBuffer;
#endif

//extern size_t xCCMFreeBytesRemaining;
//extern size_t xCCMMinimumEverFreeBytesRemaining;
extern size_t RAM_stFreeBytesRemaining;
extern size_t RAM_stMinimumEverFreeBytesRemaining;
//=============================================================================
// 公用类实现
//-----------------------------------------------------------------------------
// 完成初始化
void TInspectorServer::Init(void)
{
  
  if( nullptr == m_pucRxBuf )
    m_pucRxBuf = (uint8_t*)RAM_Malloc(SIZE_RX_BUF);
#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == m_pucRxBuf, GFC_OutOfMem );
#endif

  if( nullptr == m_pucTxBuf )
    m_pucTxBuf = (uint8_t*)RAM_Malloc(SIZE_TX_BUF);
#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == m_pucTxBuf, GFC_OutOfMem );
#endif
}
//-----------------------------------------------------------------------------
// 释放空间
void TInspectorServer::Free(void)
{
  
  if( nullptr != m_pucRxBuf )
    {
    RAM_Free( m_pucRxBuf );
    
    m_pucRxBuf = nullptr;
    }

  if( nullptr != m_pucTxBuf )
    {
    RAM_Free( m_pucTxBuf );
    
    m_pucTxBuf = nullptr;
    }
}
//-----------------------------------------------------------------------------
void TInspectorServer::OnReceive(void* pvMsg, uint32_t uNumBytes)
{

  if( nullptr == pvMsg )
    {
    uNumBytes = m_pSocket->Read(m_pucRxBuf, SIZE_RX_BUF) ;
    }
  else 
    {
    if( uNumBytes > SIZE_RX_BUF )
      uNumBytes = SIZE_RX_BUF;
    
    if( uNumBytes > 0 )
      memcpy( m_pucRxBuf, pvMsg, uNumBytes );
    }    
  
  m_uwRxCntr = uNumBytes;
}
//-----------------------------------------------------------------------------
// 定时事件
// uTick的单位是1ms
void TInspectorServer::OnTick(uint32_t uTick)
{

  if( m_uwRxCntr >= 7 )
    {
    Parser( (const uint8_t*)m_pucRxBuf, m_uwRxCntr );
      
    m_uwRxCntr = 0;
    }
  
#if AOS_ENABLE > 0
  if( STATE_TRUE == AOSBuffer.BufRdy &&
      0 != m_pSocket->StateGet(sksSentOk) ) 
    Send_ISP_AOSDATA();
#endif
}
//-----------------------------------------------------------------------------
// 解析报文
void TInspectorServer::Parser(const uint8_t *pucMsg, uint32_t uNumBytes)
{
 
    
  const TISPMsgeHeader *pMessage = nullptr;
  while( uNumBytes >= 7 )
    {
    const TISPMsgeHeader *pHeader = (const TISPMsgeHeader*)pucMsg;
      
    if( ISP_HEAD == pHeader->ucSync1 && 
        ISP_HEAD == pHeader->ucSync2 &&
        uNumBytes >= pHeader->uwLength )
      {
      uint16_t uwCRC16 = CRC16( pucMsg, pHeader->uwLength - 2, 0xFFFF );
      if( *(uint16_t*)&pucMsg[pHeader->uwLength - 2] == uwCRC16 )
        {
        pMessage = pHeader;
        break;
        }
      }

    pucMsg++;
    uNumBytes--;  
    }
    
  if( nullptr == pMessage ) 
    return ;
    
  // 帧校验正确
  switch( pMessage->ucFunc )  // FUNC
    {
    case ISP_RESET:
      {
      if( pMessage->uwLength == sizeof(TDateTimeType) + 7 )
        {
        TDateTimeType *pDTime = (TDateTimeType*)&pucMsg[5];
        DEVCLK_SetDateTime( &DevClock, pDTime );

#if AOS_ENABLE > 0
        AOSBuffer.Enable = 0;
#endif

        Send_ISP_DEVINFO();
        }
      break;
      }
      
    // 读装置资源
    case ISP_DEVRTRES:
      {
      Send_ISP_DEVRTRES();
      break;
      }
    
    case ISP_REGREAD:      
      {
      if( pMessage->uwLength == sizeof(uint32_t) + sizeof(uint16_t) + 7 )
        {
        uint32_t uRegNum = *(uint32_t*)&pucMsg[5];
        uint16_t uwCount = *(uint16_t*)&pucMsg[5 + sizeof(uint32_t)];
          
        Send_ISP_REGREAD( uRegNum, uwCount );
        }        
      
      break;
      }
      
    case ISP_REGPROP:
      {
      if( pMessage->uwLength == sizeof(uint32_t) + 7 )
        {
        uint32_t uRegNum = *(uint32_t*)&pucMsg[5];
          
        Send_ISP_REGPROP( uRegNum );
        }        
      
      break;
      }
      
    case ISP_REGWRITE:
      {
      break;
      }
      
    case ISP_REGWRTEXEC:
      {
      if( pMessage->uwLength == sizeof(uint32_t) + 7 )
        WriteExec( *(uint32_t*)pMessage->pData );
      
      break;
      }
      
      
#if AOS_ENABLE > 0
    case ISP_SETAOS:
      {
      if( uwFrameLen == sizeof(uint32_t) * 3 + 7 )
        Send_ISP_SETAOS( pucMsg + 5, uwFrameLen - 7 );
      
      break;
      }

    case ISP_CLRAOS:
      {
      if( uwFrameLen == 7 )
        {
        AOSBuffer.Enable = 0;
        AOSBuffer.BufRdy = 0;
          
        Send_ISP_CLRAOS();
        }        
      
      break;
      }
#endif

#if WAVELOGGER_EN > 0
    // 访问录波波形
    // E6 07 00 E6 30 66 D6
    case ISP_WL_DIGEST:
      {
      Send_ISP_WL_DIGEST();
        
      break;
      }

    // E6 09 00 E6 31 00 00 FA D0
    case ISP_WL_CHLINFO:
      {
      if( pMessage->uwLength == sizeof(uint16_t) + 7 )
        Send_ISP_WL_CHLINFO( *(uint16_t*)pMessage->pData );
      break;
      }

    // E6 09 00 E6 32 00 00 0A D0
    case ISP_WL_RECINFO:
      {
      if( pMessage->uwLength == sizeof(uint16_t) + 7 )
        Send_ISP_WL_RECINFO( *(uint16_t*)pMessage->pData );
      break;
      }

    // E6 0F 00 E6 33 00 00 00 00 00 00 00 00 C4 E0
    case ISP_WL_RDDATA:      
      {
      if( pMessage->uwLength == 8 + 7 )
        Send_ISP_WL_RDDATA( *(uint16_t*)&pMessage->pData[0], 
                            *(uint16_t*)&pMessage->pData[2], 
                            *(uint32_t*)&pMessage->pData[4] );
      break;
      }
      
    case ISP_WL_CLEAR:
      {
      if(  7 == pMessage->uwLength )
        {
        Send_ISP_WL_CLEAR( 16 );
        }
      else if( 16 + 7 == pMessage->uwLength )
        {
        // deocde tokens
        aes_decrypt( (const uint8_t*)SKEY_ISP, pMessage->pData, m_pucTxBuf );
        uint32_t *puTokens = (uint32_t*)(m_pucTxBuf);
        // verify tokens
        if( puTokens[1] == m_uToken &&  WL_CLEAR_TOKEN == puTokens[2] )
          {
          // 清记录
          FIX_ClearWaveLog( puTokens[3] );
            
          Send_ISP_WL_CLEAR( 4 );

          m_uToken = 0;
          }
        }
        
      break;
      }
#endif
    }

  // 喂狗
  IWDG_FeedDog();
}
//-----------------------------------------------------------------------------
// 发送报文
void TInspectorServer::SendMsg( uint8_t ucFunc, uint16_t uwInfoLen )
{
  
  if( uwInfoLen > SIZE_TX_BUF - 7 )
    return ;
  
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  pMessage->ucSync1  = ISP_HEAD;
  pMessage->ucSync2  = ISP_HEAD;
  pMessage->uwLength = uwInfoLen + 7;
  pMessage->ucFunc   = ucFunc;
  
  uint16_t uwCRC16 = CRC16( m_pucTxBuf, pMessage->uwLength - 2, 0xFFFF );
  m_pucTxBuf[pMessage->uwLength - 2] = uwCRC16;
  m_pucTxBuf[pMessage->uwLength - 1] = uwCRC16 >> 8;
  
  m_pSocket->Write( m_pucTxBuf, pMessage->uwLength ); 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 填写 ISP_DEVINFO  
void TInspectorServer::Send_ISP_DEVINFO(void)
{
  
  uint16_t  uwInfoLen = 0; //GetDevIdent( m_pucTxBuf + 5 );
  uint16_t* puwInfo = (uint16_t*)&m_pucTxBuf[uwInfoLen + 6];

  m_pucTxBuf[uwInfoLen + 5] = 9;           // 信息组数量
  puwInfo[ 0] = REG_DEVOPTION;
  puwInfo[ 1] = CNT_DevOptBits;
  puwInfo[ 2] = REG_DEVCONFIG;
  puwInfo[ 3] = CNT_DevConfigs;
  puwInfo[ 4] = REG_CALIBRATION;
  puwInfo[ 5] = CNT_Calibration;
  
  puwInfo[ 6] = REG_SWITCH;
  puwInfo[ 7] = 0;
  puwInfo[ 8] = REG_HOLDING;
  puwInfo[ 9] = 0;
  
  puwInfo[10] = REG_DEVSTATE;
  puwInfo[11] = NUM_DEVICE_STATES;
  puwInfo[12] = REG_IOSTATE;
  puwInfo[13] = NUM_IO_STATES;
  puwInfo[14] = REG_STATE;
  puwInfo[15] = NUM_STATE_REGS;
  puwInfo[16] = REG_COMMON;
  puwInfo[17] = NUM_COMMON_REGS;
  
  uwInfoLen += 18 * 2 + 1;
  
  SendMsg( ISP_DEVINFO, uwInfoLen );
}
//-----------------------------------------------------------------------------
// 发送 ISP_DEVRTRES  
void TInspectorServer::Send_ISP_DEVRTRES(void)
{
  
  uint32_t* puInfo = (uint32_t*)&m_pucTxBuf[5];

#if osCMSIS >= 0x20000U
  {
  HeapStats_t HeapStats;
  vPortGetHeapStats( &HeapStats );
  
  puInfo[ 0] = HeapStats.xAvailableHeapSpaceInBytes;
  puInfo[ 1] = HeapStats.xMinimumEverFreeBytesRemaining;
  }
  
  {
  ramHeapStats_t HeapStats;
  RAM_GetHeapStats( &HeapStats );
  puInfo[ 2] = HeapStats.xAvailableHeapSpaceInBytes;     //RAM_stFreeBytesRemaining;
  puInfo[ 3] = HeapStats.xMinimumEverFreeBytesRemaining; //RAM_stMinimumEverFreeBytesRemaining;
  puInfo[ 4] = HeapStats.xNumberOfSuccessfulFrees;       //RAM_GetFreeHeapSize();
  puInfo[ 5] = HeapStats.xMinimumEverFreeBytesRemaining; //RAM_MinimumEverFreeHeapSize();
  }
  
#else
  puInfo[ 0] = xCCMFreeBytesRemaining;
  puInfo[ 1] = xCCMMinimumEverFreeBytesRemaining;

  puInfo[ 2] = RAM_stFreeBytesRemaining;
  puInfo[ 3] = RAM_stMinimumEverFreeBytesRemaining;
  puInfo[ 4] = RAM_GetFreeHeapSize();
  puInfo[ 5] = RAM_MinimumEverFreeHeapSize();
#endif
  
  uint16_t uwInfoLen = 6 * sizeof(uint32_t);
  
  SendMsg( ISP_DEVRTRES, uwInfoLen );
}
//-----------------------------------------------------------------------------
// 发送 ISP_REGPROP  
void TInspectorServer::Send_ISP_REGREAD(uint32_t uRegNum, uint16_t uwCount)
{
    
//  if( 0 == REG_TYPE(uRegNum) )
//    return ;

//  if( REG_INDEX(uRegNum) + uwCount > DevReg_Count(uRegNum) )
//    uwCount = DevReg_Count(uRegNum) - REG_INDEX(uRegNum);

//  if( sizeof(uint16_t) * uwCount + 16 > SIZE_TX_BUF )
//    uwCount = (SIZE_TX_BUF - 16) / sizeof(uint16_t);
//  
//  if( 0 == uwCount )
//    return ;
//  
//  uint8_t* pucBuf = m_pucTxBuf + 5;
//  
//  *(uint32_t*)&pucBuf[0] = uRegNum;
//  *(uint16_t*)&pucBuf[4] = uwCount;
//  
//  uint16_t* puwData = (uint16_t*)&pucBuf[6];
//  for( uint32_t uIdx = uwCount; uIdx > 0; uIdx-- )
//    {
//    *puwData++ = DevReg_Read(uRegNum++);
//    }

//  uint16_t uwInfoLen = uwCount * sizeof(uint16_t) + 
//                       sizeof(uint16_t) + sizeof(uint32_t);
//  
//  SendMsg( ISP_REGREAD, uwInfoLen );
}
//-----------------------------------------------------------------------------
// 发送 ISP_REGPROP  
void TInspectorServer::Send_ISP_REGPROP(uint32_t uRegNum)
{
    
}
//-----------------------------------------------------------------------------
// 写寄存器
uint32_t TInspectorServer::WriteRegs( uint32_t uToken,  uint32_t uRegBeg, 
                                      uint32_t uRegNum, uint16_t* puwData )
{
  
  return 0;
}
//-----------------------------------------------------------------------------
// 执行
uint32_t TInspectorServer::WriteExec( uint32_t uToken )
{
  
  return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if AOS_ENABLE > 0
//-----------------------------------------------------------------------------
// 处理AOS任务
void TInspectorServer::Send_ISP_SETAOS(const uint8_t* pucBuf, uint16_t uwLen)
{

  AOSBuffer.Enable = 0;
  
  SendMsg( ISP_SETAOS, 0 );
  
  osDelay( 10 );
  
  uint32_t uCount = uwLen / sizeof(uint32_t);
  if( NUM_AOS_REG > uCount )
    uCount = NUM_AOS_REG;
  
  uint32_t uRegCnt = 0;
  const uint32_t* puBuf = (uint32_t*)pucBuf;
  for( uint32_t uIdx = 0; uIdx < uCount; uIdx++ )
    {
    if( REG_COMMON == REG_TYPE(puBuf[uIdx]) && 
        (puBuf[uIdx] - REG_COMMON) < NUM_COMMON_REGS )
      {
      AOSBuffer.RegNum[uRegCnt++] = puBuf[uIdx];
      }
    }      
        
  if( 0 < uRegCnt )
    {
    AOSBuffer.RegCount = uRegCnt;
    
    AOSBuffer.BufRdy  = 0;
    AOSBuffer.BufIdx  = 0;
    AOSBuffer.BufAPos = 5;
    AOSBuffer.BufBPos = 5;
      
    AOSBuffer.Enable = STATE_TRUE; 
    }
}
//-----------------------------------------------------------------------------
// 处理AOS任务
void TInspectorServer::Send_ISP_CLRAOS(void)
{

  SendMsg( ISP_CLRAOS, 0 );
}
//-----------------------------------------------------------------------------
// 处理AOS任务
void TInspectorServer::Send_ISP_AOSDATA(void)
{

  AOSBuffer.BufRdy = 0;
  
  // 取准备好的缓冲区
  uint8_t* pucBuf;
  if( 0 == AOSBuffer.BufIdx )
    pucBuf = AOSBuffer.BufferB;
  else
    pucBuf = AOSBuffer.BufferA;
  
  // 取数据长度
  uint16_t uwFrameLen = 0;
  if( 0 == AOSBuffer.BufIdx )
    uwFrameLen = AOSBuffer.BufBPos + 2;
  else
    uwFrameLen = AOSBuffer.BufAPos + 2;

  // 填写报文
  pucBuf[0] = ISP_HEAD;
  pucBuf[1] = uwFrameLen;
  pucBuf[2] = uwFrameLen >> 8;
  pucBuf[3] = ISP_HEAD;
  pucBuf[4] = ISP_AOSDATA;
  
  // 校验
  uint16_t uwCRC16 = CRC16( pucBuf, uwFrameLen - 2, 0xFFFF );
  pucBuf[uwFrameLen - 2] = uwCRC16;
  pucBuf[uwFrameLen - 1] = uwCRC16 >> 8;
  
  // 发送
  m_pSocket->Write( pucBuf, uwFrameLen );  
}
//-----------------------------------------------------------------------------
#endif // AOS_ENABLE > 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 访问录波数据
//-----------------------------------------------------------------------------
// 录波器摘要
void TInspectorServer::Send_ISP_WL_DIGEST(void)
{
  
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  uint8_t* pucBuf = pMessage->pData;
  
  pucBuf[ 0] = NUM_WAVELOG_CYCLES & 0xFF;
  pucBuf[ 1] = NUM_WAVELOG_CYCLES >> 8;
  pucBuf[ 2] = NUM_SAMPLOG_PER_PEROID;
  pucBuf[ 3] = NUM_WAVELOG_PRERECODE;
  pucBuf[ 4] = NUM_ADC_CHANNELS;
  pucBuf[ 5] = 0;
  pucBuf[ 6] = NUM_SAMPLOG_PER_CHL & 0xFF;
  pucBuf[ 7] = NUM_SAMPLOG_PER_CHL >>  8;
  pucBuf[ 8] = NUM_SAMPLOG_PER_CHL >> 16;
  pucBuf[ 9] = NUM_SAMPLOG_PER_CHL >> 24;
#if WAVELOGGER_EN > 0
  uint32_t uLogCnt = WAVLOG_GetLogCount();
#else
  uint32_t uLogCnt = 0;
#endif  
  pucBuf[10] = uLogCnt & 0xFF;
  pucBuf[11] = uLogCnt >> 8;

  SendMsg( ISP_WL_DIGEST, 12 );
}
//-----------------------------------------------------------------------------
// 录波器通道描述
void TInspectorServer::Send_ISP_WL_CHLINFO(uint16_t uwChlBeg)
{
  
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  uint8_t* pucBuf = pMessage->pData;
  uint16_t uwLen  = sizeof(uint16_t) + sizeof(FWLChlsDesp);

  memcpy( pucBuf + sizeof(uint16_t), &FWLChlsDesp, sizeof(FWLChlsDesp) );
  
  *(uint16_t*)pucBuf = uwChlBeg;
  
  SendMsg( ISP_WL_CHLINFO, uwLen );
}
//-----------------------------------------------------------------------------
// 读录波记录
void TInspectorServer::Send_ISP_WL_RECINFO(uint16_t uwRecBeg)
{

#if WAVELOGGER_EN > 0
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  uint8_t* pucBuf = pMessage->pData;
  uint16_t uwLen  = sizeof(uint16_t);
  
  TEventLogSummary aSummary;
  uint8_t* pucMsge = pucBuf + sizeof(uint16_t);
  uint32_t uLogCnt = WAVLOG_GetLogCount();

  for( uint32_t uIdx = uwRecBeg; uIdx < uLogCnt; uIdx++ )
    {
    WAVLOG_GetLogSummary( &aSummary, uIdx );
      
    pucMsge[ 0] = aSummary.Time.Year;
    pucMsge[ 1] = aSummary.Time.Year >> 8;
    pucMsge[ 2] = aSummary.Time.Month;
    pucMsge[ 3] = aSummary.Time.Day;
    pucMsge[ 4] = aSummary.Time.Hours;
    pucMsge[ 5] = aSummary.Time.Minutes;
    pucMsge[ 6] = aSummary.Time.Seconds;
    pucMsge[ 7] = aSummary.Time.milSecs;
    pucMsge[ 8] = aSummary.Time.milSecs >> 8;
    pucMsge[ 9] = aSummary.State.RegNum;
    pucMsge[10] = aSummary.State.RegNum >> 8;
    pucMsge[11] = aSummary.State.Action;
    pucMsge[12] = aSummary.State.Type;
      
    uwLen   += 13;
    pucMsge += 13;
      
    // 测试是否还有足够容量  
    if( uwLen + 13 > SIZE_TX_BUF - 10 )
      break;
    }
    
  *(uint16_t*)pucBuf = uwRecBeg;

  SendMsg( ISP_WL_RECINFO, uwLen );
#endif
}
//-----------------------------------------------------------------------------
// 读录波录波数据
// 要求的波形数据字节位置
void TInspectorServer::Send_ISP_WL_RDDATA( uint16_t uwRecNo, 
                                           uint16_t uwChlNo, 
                                           uint32_t uPosition  
                                         )
{
  
#if WAVELOGGER_EN > 0
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  uint8_t* pucBuf = pMessage->pData;
  uint16_t uwLen  = sizeof(uint32_t) * 2;
  
  uint32_t uBytesRead = 0;
  if( uwRecNo < WAVLOG_GetLogCount() && 
      uwChlNo < NUM_FWLTracksDesp )
    {
    uint32_t uSampsToRead, uSamplePos;
    
    const GWaveLogTrackDesp* pTrack = &FWLChlsDesp[uwChlNo];
    if( 0 == pTrack->ucChlType )
      {
      // 模拟通道
      uSampsToRead = (SIZE_TX_BUF - 10 - uwLen) / sizeof(uint16_t);
      uSamplePos   = uPosition / sizeof(uint16_t);
        
      if( SIZE_SAMPLOG_ADCCHL * sizeof(uint16_t) <= uPosition )
        uSampsToRead = 0;
      }
    else
      {
      // 数字通道
      uSampsToRead = (SIZE_TX_BUF - 10 - uwLen) * 8;
      uSamplePos   = uPosition * 8;

      if( SIZE_WAVELOG_BINCHL <= uPosition )
        uSampsToRead = 0;
      }

    // 读录波数据
    uBytesRead = WAVLOG_ReadChlData( uwRecNo, 
                                     uwChlNo, 
                                     uSamplePos,
                                     pucBuf + uwLen, 
                                     uSampsToRead );
    
    uwLen += uBytesRead;
    }
   
  //     
  *(uint16_t*)&pucBuf[0] = uwChlNo;
  *(uint16_t*)&pucBuf[2] = uBytesRead;
  *(uint32_t*)&pucBuf[4] = uPosition;
  
  SendMsg( ISP_WL_RDDATA, uwLen );
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 读录波记录                              
void TInspectorServer::Send_ISP_WL_CLEAR( uint16_t len )
{
  
  TISPMsgeHeader *pMessage = (TISPMsgeHeader*)m_pucTxBuf;
  uint8_t* pucBuf = pMessage->pData;
  uint16_t uwLen  = len;
  
  if( 16 == uwLen )
    {
    uint32_t *puData = (uint32_t*)m_pucRxBuf;
      
    m_uToken  = RNG_GetRand();
    puData[0] = TOKEN_FIX_OPERATE;
    puData[1] = WL_CLEAR_TOKEN;
    puData[2] = RNG_GetRand();
    puData[3] = m_uToken;
    
      
    aes_encrypt( (const uint8_t*)SKEY_ISP, m_pucRxBuf, pucBuf );
    }
  else if( 4 == uwLen )
    {
#if WAVELOGGER_EN > 0
    *(uint32_t*)pucBuf = WAVLOG_GetLogCount();
#else
    *(uint32_t*)pucBuf = 0;
#endif
    }

  SendMsg( ISP_WL_CLEAR, uwLen );
}
//-----------------------------------------------------------------------------
