//-----------------------------------------------------------------------------
/*
 File        : SocketBase.cpp
 Version     : V1.10
 By          : 银网科技

 Description :通讯Socket基类
        
 Date       : 2023.12.05 2015.12.12
 
 2024.1.15
   v2.0
   代码规范
*/
//-----------------------------------------------------------------------------
#include "SocketBase.h"

#include "Protocol.h"
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
// 公用类实现
//-----------------------------------------------------------------------------
// class TCOMSocket
//-----------------------------------------------------------------------------
// 析构
TCOMSocket::~TCOMSocket()
{
  
  SetProtocol( nullptr );
}
//-----------------------------------------------------------------------------
// 释放空间，准备删除本对象
void TCOMSocket::Free()
{
}
//-----------------------------------------------------------------------------
void TCOMSocket::SetProtocol( TCOMProtocol *pProtocol )
{
  
  
  if( pProtocol != m_pProtocol )
    {
    if( nullptr != m_pProtocol )
      m_pProtocol->SetSocket( nullptr ); 
      
    if( nullptr != pProtocol )
      pProtocol->SetSocket( this );

    m_pProtocol = pProtocol;
    }
}
//----------------------------------------------------------------------------
void TCOMSocket::Init()
{
  
  m_uState    = 0;
  m_uNextTick = 0;
  
  if( nullptr != m_pProtocol )
    m_pProtocol->Init();    
}
//----------------------------------------------------------------------------
void TCOMSocket::OnConnect(void)
{
  
  StateClr( sksTimeOut );
  StateSet(sksConn | sksSentOk);

  if( nullptr != m_pProtocol )
    m_pProtocol->OnConnect();
}
//----------------------------------------------------------------------------
void TCOMSocket::OnDisconnect(void)
{
  
  StateClr(sksConn );
  
  if( nullptr != m_pProtocol )
    m_pProtocol->OnDisconnect();
}
//----------------------------------------------------------------------------
void TCOMSocket::OnTimeout(void)
{
  
  StateSet(sksTimeOut);
  
  if( nullptr != m_pProtocol )
    m_pProtocol->OnTimeout();
}
//----------------------------------------------------------------------------
void TCOMSocket::OnSent(void)
{
  
  StateSet(sksSentOk);

  if( nullptr != m_pProtocol )
    m_pProtocol->OnSent();
}
//----------------------------------------------------------------------------
void TCOMSocket::OnReceive(void)
{
  
  if( nullptr != m_pProtocol )
    m_pProtocol->OnReceive();
}
//-----------------------------------------------------------------------------
void TCOMSocket::OnReceive( const void* pvMsg, uint32_t uNumBytes )
{
  
  if( nullptr != m_pProtocol )
    m_pProtocol->OnReceive( pvMsg, uNumBytes );
}
//-----------------------------------------------------------------------------
void TCOMSocket::OnTick(uint32_t uTick)
{

  if( nullptr != m_pProtocol )
    m_pProtocol->OnTick( uTick );
}
//-----------------------------------------------------------------------------
