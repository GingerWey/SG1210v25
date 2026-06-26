//-----------------------------------------------------------------------------
/*
 File        : EchoSvr.h
 Version     : V1.10
 By          : 银网科技

 Description :Echo服务器协议实现

 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "EchoSvr.h"

#include "RamHeap.h"
#include "COMBuffer.h"

#include "SocketBase.h"

#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define SIZE_RX_BUF    256

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
inline void* operator new(size_t, void* ptr )
  {  return ptr; }
//=============================================================================
// 公用类实现
//-----------------------------------------------------------------------------
// 完成初始化
void TEchoServer::Init(void)
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
    m_pucTxBuf = (uint8_t*)RAM_Malloc(SIZE_RX_BUF);
#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == m_pucTxBuf, GFC_OutOfMem );
#endif

  m_uRecvBytes = 0;
}
//-----------------------------------------------------------------------------
// 释放空间
void TEchoServer::Free(void)
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

  m_uRecvBytes = 0;
}
//-----------------------------------------------------------------------------
void TEchoServer::OnReceive(const void* pvMsg, uint32_t uNumBytes)
{

  if( nullptr != m_pSocket )
    {
    if( nullptr != pvMsg && 0 < uNumBytes )
      {
      m_pRxBuffer->Write( pvMsg, uNumBytes );
      m_uRecvBytes += uNumBytes;
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
      m_pSocket->StateSet( sksNeedSend );
    }
    
  OnTick( 0 );
}
//-----------------------------------------------------------------------------
// 定时事件
// uTick的单位是1ms
void TEchoServer::OnTick(uint32_t uTick)
{

  m_uRecvBytes = m_pRxBuffer->Used();
  if( SIZE_RX_BUF < m_uRecvBytes )
    m_uRecvBytes = SIZE_RX_BUF;

  if( m_uRecvBytes > 0 )
    {
    m_uRecvBytes = m_pRxBuffer->Read( m_pucTxBuf, m_uRecvBytes );

    //m_pRxBuffer->AdvanceReadIndex(m_uRecvBytes);  // 用RingBuffer导致GUI锁死，原因不明
    m_pRxBuffer->Reset();  // 直接Reset
    }

  if( m_uRecvBytes > 0 )
    {
    if( m_pSocket->Write(m_pucTxBuf, m_uRecvBytes) )
      {
      m_uRecvBytes = 0;

      m_pSocket->StateClr( sksNeedSend );
      }
    }

//  m_pSocket->Write(FTestStr, 24);
}
//-----------------------------------------------------------------------------
