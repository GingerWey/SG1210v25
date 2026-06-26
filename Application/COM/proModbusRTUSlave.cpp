//-----------------------------------------------------------------------------
/*
 File        : proModbusRTUSlave.cpp
 Version     : V1.10
 By          : 银网科技

 Description :Modbus RTU Slave协议对象实现

 Date       : 2024.1.18
*/
//-----------------------------------------------------------------------------
#include "proModbusRTUSlave.h"

#include "RamHeap.h"
#include "COMBuffer.h"

#include "SocketBase.h"

#include "DevFixed.h"
#include "DevRegs.h"
#include "DevDebug.h"

#include "rtc.h"
#include "DevClock.h"

#include "RNGen.h"
#include "CRC1632.h"

#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 报文缓冲区尺寸
#define SIZE_RX_BUF               264
#define SIZE_TX_BUF               256
//-----------------------------------------------------------------------------
// Modbus的广播地址
#define BROADCAST_ADDR            0xFF
//-----------------------------------------------------------------------------
// 功能码      
#define FUNCODE_COIL_RD           0x01     // 读线圈寄存器      位
#define FUNCODE_COIL_WR           0x05     // 写单个线圈寄存器  位
#define FUNCODE_COIL_WRM          0x0F     // 写多个线圈寄存器  位

#define FUNCODE_DI_RD             0x02     // 读离散寄存器      位
      
#define FUNCODE_HOLDING_RD        0x03     // 读保持寄存器      字
#define FUNCODE_HOLDING_WR        0x06     // 写单个保持寄存器  字
#define FUNCODE_HOLDING_WRM       0x10     // 写多个保持寄存器  字  校时

#define FUNCODE_AI_RD             0x04     // 读输入寄存器      字

#define FUNCODE_FILE_RD           0x14     // 读文件

#define FUNCODE_EVENTLOG_RD       0x0C     // 读SOE
//-----------------------------------------------------------------------------
// 出错代码
// 迭加在FUNC上的错误标识
#define ILLEGAL_REQUEST           0x80     // 无效请求

#define ERROR_FUNC_NOTRPOVIDED    0x01
#define ERROR_ADDR_OVERFLOW       0x02
#define ERROR_DATANUM_INCORRECT   0x03
#define ERROR_DATA_INCORRECT      0x04
#define ERROR_CMD_REJECTED        0x05
//-----------------------------------------------------------------------------
// 测量值的最大表示
#define MEASURE_MAX_MEANS         32000
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
#define NUM_Element(x)    (sizeof(x)/ sizeof((x)[0]))
//=============================================================================
// 局部数据结构
//-----------------------------------------------------------------------------
typedef struct tagCOMRegInfo
{
  uint16_t            RegNum;
  uint16_t            Scale    = 0;
  uint32_t            UpLimit  = 0;
} TCOMRegInfo;

typedef struct tagCOMRegInfoList
{
  uint16_t            Type;
  uint16_t            Count;
    
  const TCOMRegInfo  *pInfos;
} TCOMRegInfoList;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 定值信息表
constexpr TCOMRegInfo _criHoldingList[] =
{
   {
   .RegNum = REG_AUTOCTRL_EN,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_AUTO_TURNOFF,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_ACTION_VOLTAGE,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_COIL_VOLTAGE,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_PWRON_TIME,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_PWROFF_TIME,
   .Scale  = 1
   }
  ,{
   .RegNum = REG_SHUTDOWN_TIME,
   .Scale  = 1
   }
};
#define NUM_criHoldingList    NUM_Element(_criHoldingList)

constexpr TCOMRegInfoList COMHoldingList = 
{
  .Type   = 0,
  .Count  = NUM_criHoldingList,
  .pInfos = _criHoldingList
};
//-----------------------------------------------------------------------------
// 模拟量输入表
constexpr TCOMRegInfo _criAIList[] =
{
   {
   .RegNum  = REG_RL_Uin,
   .Scale   = 1,
   .UpLimit = 600
   }
  ,{
   .RegNum  = REG_RL_UinVal,
   .Scale   = 1,
   .UpLimit = 600
   }
  ,{
   .RegNum  = REG_RL_ACFreq,
   .Scale   = 1,
   .UpLimit = 60
   }
  ,{
   .RegNum  = REG_RL_Uout,
   .Scale   = 1,
   .UpLimit = 600
   }
  ,{
   .RegNum  = REG_RL_UoutVal,
   .Scale   = 1,
   .UpLimit = 600
   }
  ,{
   .RegNum  = REG_RL_VOFreq,
   .Scale   = 1,
   .UpLimit = 60
   }
  ,{
   .RegNum  = REG_RL_BCHRG_Ubus,
   .Scale   = 1,
   .UpLimit = 10
   }
  ,{
   .RegNum = REG_RL_BCHRG_Ibus,
   .Scale  = 1,
   .UpLimit = 5000
   }
  ,{
   .RegNum  = REG_RL_BCHRG_Level,
   .Scale   = 1,
   .UpLimit = 100
   }
  ,{
   .RegNum  = REG_RL_BTOUT_Ubus,
   .Scale   = 1,
   .UpLimit = 10
   }
  ,{
   .RegNum = REG_RL_BTOUT_Ibus,
   .Scale  = 1,
   .UpLimit = 40000
   }
  ,{
   .RegNum  = REG_RL_BTOUT_Pbus,
   .Scale   = 1,
   .UpLimit = 10000
   }
  ,{
   .RegNum  = REG_RL_COIL_Volt,
   .Scale   = 1,
   .UpLimit = 100
   }
  ,{
   .RegNum  = REG_RL_RTC_TEMP,
   .Scale   = 1,
   .UpLimit = 200
   }
  ,{
   .RegNum  = REG_RL_BAT_TEMPERATRUE,
   .Scale   = 1,
   .UpLimit = 200
   }
};
#define NUM_criAIList         NUM_Element(_criAIList)

constexpr TCOMRegInfoList COMAIList = 
{
  .Type   = 0,
  .Count  = NUM_criAIList,
  .pInfos = _criAIList
};
//-----------------------------------------------------------------------------
// 数字量表
constexpr TCOMRegInfo _criDIList[] =
{
   {
   .RegNum  = 0             // 事件总信号
   }
  ,{
   .RegNum  = REG_DI0
   }
  ,{
   .RegNum  = REG_DI1
   }
  ,{
   .RegNum  = REG_DI2
   }
  ,{
   .RegNum  = REG_DI3
   }
  ,{
   .RegNum  = REG_DI4
   }
  ,{
   .RegNum  = REG_DI5
   }
  ,{
   .RegNum  = REG_DI6
   }
};
#define NUM_criDIList         NUM_Element(_criDIList)

constexpr TCOMRegInfoList COMDIList = 
{
  .Type   = 0,
  .Count  = NUM_criDIList,
  .pInfos = _criDIList
};
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法申明
//-----------------------------------------------------------------------------
inline void* operator new(size_t, void* ptr )
  {  return ptr; }
//=============================================================================
// 公用类实现
//-----------------------------------------------------------------------------
// 完成初始化
void TModbusRTUSlave::Init()
{
  
  if( nullptr == m_pRxBuffer )
    {
    void* pvMem = RAM_Malloc( sizeof(TCOMBuffer) + 8 );
#ifdef USE_DEV_ASSERT
    DEV_ASSERT( nullptr == pvMem, GFC_OutOfMem );
#endif
    
    m_pRxBuffer = new (pvMem)TCOMBuffer( RAM_Malloc(SIZE_RX_BUF + 8),
                                         SIZE_RX_BUF );
    }
#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == m_pRxBuffer, GFC_OutOfMem );
#endif

  if( nullptr == m_pucTxBuf )
    {
    m_pucTxBuf = (uint8_t*)RAM_Malloc(SIZE_TX_BUF);

#ifdef USE_DEV_ASSERT
    DEV_ASSERT( nullptr == m_pucTxBuf, GFC_OutOfMem );
#endif
    }
}
//-----------------------------------------------------------------------------
// 释放空间
void TModbusRTUSlave::Free()
{

  if( nullptr != m_pRxBuffer && nullptr != m_pRxBuffer->getBuffer() )
    RAM_Free( m_pRxBuffer->getBuffer() );

  if( nullptr != m_pRxBuffer )
    {
    RAM_Free( m_pRxBuffer );

    m_pRxBuffer = nullptr;
    }

  if( nullptr != m_pucTxBuf )
    {
    RAM_Free( m_pucTxBuf );

    m_pucTxBuf = nullptr;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 定时事件
// uTick的单位是1ms
void TModbusRTUSlave::OnTick(uint32_t uTick)
{
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 接收到报文
void TModbusRTUSlave::OnReceive( const void* pvMsg, uint32_t uNumBytes )
{

  if( nullptr != m_pSocket )
    {
    if( nullptr != pvMsg && 0 < uNumBytes )
      {
      m_pRxBuffer->Write( pvMsg, uNumBytes );
      }
    else
      {
      if( SIZE_RX_BUF < uNumBytes )
        uNumBytes = SIZE_RX_BUF;

      uNumBytes = m_pSocket->Read(m_pucTxBuf, uNumBytes);
      if( uNumBytes > 0 )
        m_pRxBuffer->Write( m_pucTxBuf, uNumBytes );
      }

    if( 0 < uNumBytes )
      parseMessage();
    }
}
//-----------------------------------------------------------------------------
// 解析报文
void TModbusRTUSlave::parseMessage()
{

  const TUARTConfig *poUART = GET_UARTOPT(0);
  uint32_t uSlavdeID = poUART->Addr;
  
  while( true )
    {
    const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
    uint32_t uMsgLength   = m_pRxBuffer->Used(),
             uBytesSkiped = 0;
    // 在接收报文中搜索帧头
    while( 4 <= uMsgLength - uBytesSkiped )
      {
      if(  pMsgBuff[uBytesSkiped] == uSlavdeID ||
          // 只有校时命令可以广播
          (pMsgBuff[uBytesSkiped] == BROADCAST_ADDR &&  
           pMsgBuff[uBytesSkiped + 1] == FUNCODE_HOLDING_WRM ) )
        {
        break;
        }

      uBytesSkiped++;
      }

    // 缓冲区内报文长度不够， 搜索失败
    if( 8 > uMsgLength - uBytesSkiped )
      {
      m_pRxBuffer->DiscardReadMessage( uBytesSkiped );
      break;
      }
    else
      {
      uMsgLength -= uBytesSkiped;
      pMsgBuff   += uBytesSkiped;

      if( 0 < uBytesSkiped )
        m_pRxBuffer->AdvanceReadIndex( uBytesSkiped );
      }
    
    uint32_t uLength;
    switch( pMsgBuff[1] )
      {
      case FUNCODE_COIL_RD:     // 0x01
      case FUNCODE_COIL_WR:     // 0x05
      case FUNCODE_DI_RD:       // 0x02
      case FUNCODE_HOLDING_RD:  // 0x03
      case FUNCODE_HOLDING_WR:  // 0x06
      case FUNCODE_AI_RD:       // 0x04
        {
        uLength = 8;
        break;
        }

      case FUNCODE_COIL_WRM:    // 0x0F
      case FUNCODE_HOLDING_WRM: // 0x10
        {
        uLength = 9 + pMsgBuff[6];
        break;
        }

      case FUNCODE_EVENTLOG_RD:
        {
        uLength = 4;
        break;
        }

      default:
        {
        uLength = 0;
        break;
        }
      }

    if( 0 < uLength )
      {
      uint32_t uCRC16 = m_pRxBuffer->Execute( CRC16, uLength - 2 );
      if( m_pRxBuffer->GetWordLH(uLength - 2) == uCRC16 )
        {
        // 分发报文到功能解析器
        dispatch();
          
        // 跳过当前报文
        m_pRxBuffer->AdvanceReadIndex( uLength );
        }
      else
        // CRC校验不通过，跳过1字节
        m_pRxBuffer->AdvanceReadIndex( 1 );
      }
    else
      // 功能码不识别，跳过1字节
      m_pRxBuffer->AdvanceReadIndex( 1 );
    }

  // 清理已扫描过的报文
  m_pRxBuffer->DiscardReadMessage();
}
//-----------------------------------------------------------------------------
// 分配报文解析
void TModbusRTUSlave::dispatch()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  
  memset( m_pucTxBuf, 0, SIZE_TX_BUF );
  m_pucTxBuf[0] = pMsgBuff[0];
  m_pucTxBuf[1] = pMsgBuff[1];

  bool bResult;
  switch( pMsgBuff[1] )
    {
//    case FUNCODE_COIL_RD:     // 0x01
//      {
//      bResult = coilRead();
//      break;
//      }

    case FUNCODE_DI_RD:       // 0x02
      {
      bResult = discreteInputsRead();
      break;
      }

    case FUNCODE_HOLDING_RD:  // 0x03
      {
      bResult = holdingRead();
      break;
      }

    case FUNCODE_AI_RD:       // 0x04
      {
      bResult = inputRead();
      break;
      }

//    case FUNCODE_COIL_WR:     // 0x05
//      {
//      bResult = coilWrite();
//      break;
//      }

//    case FUNCODE_HOLDING_WR:  // 0x06
//      {
//      bResult = holdingWrite();
//      break;
//      }
      
    case FUNCODE_EVENTLOG_RD:
      {
      bResult = eventlogRead();
      break;
      }

//    case FUNCODE_COIL_WRM:    // 0x0F
//      {
//      bResult = coilMutipleWrite();
//      break;
//      }

    case FUNCODE_HOLDING_WRM: // 0x10
      {
      bResult = holdingMutipleWrite();
      break;
      }

    default:
      {
      // 功能未提供
      m_pucTxBuf[1] |= ILLEGAL_REQUEST;
      m_pucTxBuf[2]  = ERROR_FUNC_NOTRPOVIDED;
      sendMessage( 3 );

      bResult = false;
      break;
      }
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 发送报文
// 输入：
//   uLength: 报文长度，不含校验域
void TModbusRTUSlave::sendMessage(uint32_t uLength)
{

  uint32_t uCRC16 = CRC16( m_pucTxBuf, uLength );
  m_pucTxBuf[0 + uLength] = uCRC16;
  m_pucTxBuf[1 + uLength] = uCRC16 / 0x100;
  
  m_pSocket->Write( m_pucTxBuf, uLength + 2 );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 读线圈寄存器  0x01
bool TModbusRTUSlave::coilRead()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;


  return bResult;
}
//-----------------------------------------------------------------------------
// 写单个线圈寄存器  0x05
bool TModbusRTUSlave::coilWrite()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;

  return bResult;
}
//-----------------------------------------------------------------------------
// 写多个线圈寄存器  0x0F
bool TModbusRTUSlave::coilMutipleWrite()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

 bool bResult = false;

  return bResult;
}
//-----------------------------------------------------------------------------
// 读离散寄存器  0x02
bool TModbusRTUSlave::discreteInputsRead()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;

  uint32_t uLength;
  if( uRegStart + uRegStart >= COMDIList.Count )
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_ADDR_OVERFLOW;
    uLength = 3;
    }
  else if( (SIZE_TX_BUF - 8) < (uDataNum + 7) / 8 )
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_DATANUM_INCORRECT;
    uLength = 3;
    }
  else
    {
    uint8_t ucMask = 1, ucData = 0;
 
    uLength = 3;
    m_pucTxBuf[2]  = (uDataNum + 7) / 8;

    for( uint32_t uIdx = 0; uIdx < uDataNum; uIdx++ )
      {
      if( COMDIList.Count > uIdx + uRegStart )
        {
        const TCOMRegInfo *pInfo = COMDIList.pInfos + uIdx + uRegStart;
        if( 0 == pInfo->RegNum && GetNewEventCount() > 0 )
          ucData |= ucMask;  // 取事件总信号
        else
          {
          TStateReg stValue = _GetIOStateReg( pInfo->RegNum );
          if( STATE_TRUE == stValue )
            ucData |= ucMask;
          }
        }

      ucMask <<= 1;
      if( 0 == ucMask )
        {
        m_pucTxBuf[uLength++] = ucData;

        ucData = 0;
        ucMask = 1;
        }
      }

    if( 1 < ucMask )
      m_pucTxBuf[uLength++] = ucData;

    bResult = true;
    }

  // 发送回复
  sendMessage( uLength );

  return bResult;
}
//-----------------------------------------------------------------------------
// 读保持寄存器  0x03
bool TModbusRTUSlave::holdingRead()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;

  uint32_t uLength;
  
  // 区分读数据的类型
  const TCOMRegInfoList *pList;
  if( 100 <= uRegStart  && uRegStart < 200 )
    {
    pList = &COMHoldingList;
    
    uRegStart -= 100;
    }
  else if( 100 > uRegStart )
    {
    pList = &COMAIList;
    }
  else
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_ADDR_OVERFLOW;
    uLength = 3;

    pList = nullptr;
    }
  
  // 是否在有效的地址范围？
  if( nullptr != pList )
    {
    // 
    if( uRegStart > pList->Count )
      {
      m_pucTxBuf[1] |= ILLEGAL_REQUEST;
      m_pucTxBuf[2]  = ERROR_ADDR_OVERFLOW;
      uLength = 3;
      }
    // 数据个数不会溢出？
    else if( (SIZE_TX_BUF - 8 ) / 2 < uDataNum )
      {
      m_pucTxBuf[1] |= ILLEGAL_REQUEST;
      m_pucTxBuf[2]  = ERROR_DATANUM_INCORRECT;
      uLength = 3;
      }
    else
      {
      // 填写数据
      m_pucTxBuf[2] = uDataNum * 2;
      for( uint32_t uIdx = 0; uIdx < uDataNum; uIdx++ )
        {
        uint32_t uValue;
        if( pList->Count > uIdx + uRegStart )
          {
          const TCOMRegInfo *pInfo = pList->pInfos + uIdx + uRegStart;
            
          if( pList == &COMHoldingList )
            uValue = _GetDevCfgReg(pInfo->RegNum);
          else if( pList == &COMAIList )
            {
#ifdef REG_REAL
            if( REG_TYPE(pInfo->RegNum) == REG_COMMON )
#endif
              {
              int iValue = (int)_GetCommonReg(pInfo->RegNum);
              // 数值归一化
              if( 0 < pInfo->Scale && 0 < pInfo->UpLimit)
                {
                float fValue = (float)iValue / pInfo->Scale;
                iValue = fValue * MEASURE_MAX_MEANS / pInfo->UpLimit + 0.5f;
                uValue = (uint16_t)iValue;
                }
              else
                uValue = 0;
              }
#ifdef REG_REAL
            else if( REG_TYPE(pInfo->RegNum) == REG_REAL )
              {
              float fValue = (int)_GetRealReg(pInfo->RegNum);
              // 数值归一化
              if( 0 < pInfo->Scale && 0 < pInfo->UpLimit)
                {
                fValue = fValue / pInfo->Scale;
                int iValue = fValue * MEASURE_MAX_MEANS / pInfo->UpLimit + 0.5f;
                uValue = (uint16_t)iValue;
                }
              else
                uValue = 0;
              }
#else      
            uValue = 0;
#endif    
            }
          }
        else
          uValue = 0;
        
        m_pucTxBuf[3 + uIdx * 2] = uValue / 0x100;
        m_pucTxBuf[4 + uIdx * 2] = uValue;
        }
    
      uLength = 3 + uDataNum * 2;

      bResult = true;
      }
    }
  
  // 发送
  sendMessage( uLength );

  return bResult;
}
//-----------------------------------------------------------------------------
// 写保持寄存器  0x06
bool TModbusRTUSlave::holdingWrite()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;

  return bResult;
}
//-----------------------------------------------------------------------------
// 读保持寄存器  0x10
bool TModbusRTUSlave::holdingMutipleWrite()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;
  
  uint32_t uLength = 0;
  // Addr: 0-15是校时区
  if( 0 == uRegStart )
    {
    // 校时命令必须一次传完
    if( 7 != uDataNum || 14 != pMsgBuff[6] )
      {
      m_pucTxBuf[1] |= ILLEGAL_REQUEST;
      m_pucTxBuf[2]  = ERROR_DATANUM_INCORRECT;
      uLength = 3;
      }
    else
      {
      TDateTimeType dtTime;
      dtTime.Year       = (uint32_t)pMsgBuff[7] * 0x100 + pMsgBuff[8];
      dtTime.Month      = pMsgBuff[10];
      dtTime.Day        = pMsgBuff[12];
      dtTime.Hours      = pMsgBuff[14];
      dtTime.Minutes    = pMsgBuff[16];
      dtTime.Seconds    = pMsgBuff[18];
      dtTime.MilSeconds = (uint32_t)pMsgBuff[19] * 0x100 + pMsgBuff[20];
        
      if( 0 == DEVCLK_CheckDateTime( &dtTime ) )
        {
        if( 0 != RTC_SetTime( &dtTime ) )
          {
          m_pucTxBuf[1] |= ILLEGAL_REQUEST;
          m_pucTxBuf[2]  = ERROR_CMD_REJECTED;   // 命令被拒绝
          uLength = 3;
          }
        else
          {
          // 校时命令正确完成
          memcpy( m_pucTxBuf + 2, pMsgBuff + 2, 4 );
          uLength = 6;
          }

        bResult = true;
        }
      else
        {
        m_pucTxBuf[1] |= ILLEGAL_REQUEST;
        m_pucTxBuf[2]  = ERROR_DATA_INCORRECT;  // 数据不正确
        uLength = 3;
        }
      }
    }
  else //if( uRegStart < 16 )
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_ADDR_OVERFLOW;
    uLength = 3;
    }

    //  如果长度允许，则发送
  if( 0 < uLength && BROADCAST_ADDR != m_pucTxBuf[0] )
    {
    sendMessage( uLength );
    }

  return bResult;
}
//-----------------------------------------------------------------------------
// 读输入寄存器  0x04
bool TModbusRTUSlave::inputRead()
{

  const uint8_t* pMsgBuff = m_pRxBuffer->MessageBuffer();
  uint32_t uRegStart = (uint32_t)pMsgBuff[2] * 0x100 + pMsgBuff[3],
           uDataNum  = (uint32_t)pMsgBuff[4] * 0x100 + pMsgBuff[5];

  bool bResult = false;

  uint32_t uLength;
  if( uRegStart + uRegStart >= COMAIList.Count )
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_ADDR_OVERFLOW;
    uLength = 3;
    }
  else if( (SIZE_TX_BUF - 8 ) / 2 < uDataNum )
    {
    m_pucTxBuf[1] |= ILLEGAL_REQUEST;
    m_pucTxBuf[2]  = ERROR_DATANUM_INCORRECT;
    uLength = 3;
    }
  else
    {
    m_pucTxBuf[2] = uDataNum * 2;
    for( uint32_t uIdx = 0; uIdx < uDataNum; uIdx++ )
      {
      uint32_t uValue;
      if( COMAIList.Count > uIdx + uRegStart )
        {
        const TCOMRegInfo *pInfo = COMAIList.pInfos + uIdx + uRegStart;
#ifdef REG_REAL
        if( REG_TYPE(pInfo->RegNum) == REG_COMMON )
#endif
          {
          int iValue = (int)_GetCommonReg(pInfo->RegNum);
          // 数值归一化
          if( 0 < pInfo->Scale && 0 < pInfo->UpLimit)
            {
            float fValue = (float)iValue / pInfo->Scale;
            iValue = fValue * MEASURE_MAX_MEANS / pInfo->UpLimit + 0.5f;
            uValue = (uint16_t)iValue;
            }
          else
            uValue = 0;
          }
#ifdef REG_REAL
        else if( REG_TYPE(pInfo->RegNum) == REG_REAL )
          {
          float fValue = (int)_GetRealReg(pInfo->RegNum);
          // 数值归一化
          if( 0 < pInfo->Scale && 0 < pInfo->UpLimit)
            {
            fValue = fValue / pInfo->Scale;
            int iValue = fValue * MEASURE_MAX_MEANS / pInfo->UpLimit + 0.5f;
            uValue = (uint16_t)iValue;
            }
          else
            uValue = 0;
          }
#else
        uValue = 0;
#endif
        }
      else
        uValue = 0;
      
      m_pucTxBuf[3 + uIdx * 2] = uValue / 0x100;
      m_pucTxBuf[4 + uIdx * 2] = uValue;
      }

    uLength = 3 + uDataNum * 2;

    bResult = true;
    }

  // 发送报文
  sendMessage( uLength );

  return bResult;
}
//-----------------------------------------------------------------------------
// 读SOE 0x0C
//    Byte Order         Field Name
//  ---------------    -------------------------------
//        0              Slave Id
//        1              Function Code  (0x0c)
//        2              Byte Count     (13)
//        3              Time.Year    Since 2000
//        4              Time.Month
//        5              Time.Day
//        6              Time.Hour
//        7              Time.Minute
//        8              Time.Second
//        9              Time.MilSecond-Hi
//       10              Time.MilSecond-Lo
//       11              Event Type
//       12              Event Index
//       13              Event Action
//       14              Event Data.Hi
//       15              Event Data.Lo
//       16              CRC16.Lo
//       17              CRC16.Hi
bool TModbusRTUSlave::eventlogRead()
{
  
  bool bResult = false;
  
  // 
  const TCOMEventItem *pEvent = FetchEvent();
  // 读Event   todo
  
  // 填写事件
  uint32_t uLength;
  if( nullptr != pEvent )
    {
    m_pucTxBuf[2]  = 13;
    m_pucTxBuf[3]  = pEvent->Summary.Time.Year - 2000;
    m_pucTxBuf[4]  = pEvent->Summary.Time.Month;
    m_pucTxBuf[5]  = pEvent->Summary.Time.Day;
    m_pucTxBuf[6]  = pEvent->Summary.Time.Hours;
    m_pucTxBuf[7]  = pEvent->Summary.Time.Minutes;
    m_pucTxBuf[8]  = pEvent->Summary.Time.Seconds;
    m_pucTxBuf[9]  = pEvent->Summary.Time.milSecs / 0x100;
    m_pucTxBuf[10] = pEvent->Summary.Time.milSecs;
   
    m_pucTxBuf[11] = pEvent->Summary.State.Type;
    m_pucTxBuf[12] = 0;
    for( int iIdx = 1; iIdx < COMDIList.Count; iIdx++ )
      {
      if( COMDIList.pInfos[iIdx].RegNum == pEvent->Summary.State.RegNum )
        m_pucTxBuf[11] = iIdx;
      }
    m_pucTxBuf[13] = pEvent->Summary.State.Action;

//    float fValue = (float)(pEvent->Data[0]) / RATIO_Voltage;
    int   iValue =  pEvent->Data[0] * MEASURE_MAX_MEANS / 600.0f + 0.5f;
    m_pucTxBuf[14] = iValue / 0x100;
    m_pucTxBuf[15] = iValue;
      
    uLength = m_pucTxBuf[2] + 5;

    bResult = true;
    }
  else
    {
    // 没有新事件
    m_pucTxBuf[2] = 0;
      
    uLength = 3;
    }

  sendMessage( uLength );

  return bResult;
}
//-----------------------------------------------------------------------------
