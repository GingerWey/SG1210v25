//-----------------------------------------------------------------------------
/*
 File        : SocketBase.h
 Version     : V1.10
 By          : 银网科技

 Description :通讯Socket基类
        
 Date       : 2023.12.05 2015.12.12
 
 2024.1.15
   v2.0
   代码规范
 
 ------------------------------------------------------------------------------
 结构说明：
          通讯端口                             解析器
        TCOMSocket                         TCOMProtocol
             |                                   |
             |--OnReceive     -->     OnReceive--|
             |--Write         <--         Write--|
             |--Disconnect    <--    Disconnect--|
             |                                   |
             |--OnConnect     -->     OnConnect--|    
             |--OnDisconnect  -->  OnDisconnect--|
             |--OnTimeout     -->     OnTimeout--|
             |--OnTick        -->        OnTick--|
*/
//-----------------------------------------------------------------------------
#ifndef __SOCKET_BASE_H
#define __SOCKET_BASE_H

#include <cstdint>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 公用宏
//-----------------------------------------------------------------------------

//=============================================================================
// 公用数据结构
//-----------------------------------------------------------------------------
// Socket的当前状态
typedef enum tagSocketState
{

  sksIdle      = 0,       // 空闲
  sksInit      = 0x01,    // 完成初始化 
  sksConn      = 0x02,    // 完成连接,可以正常传输数据
  sksAccepted  = 0x04,    // 已收受连接
  sksSentOk    = 0x08,    // 发送完成
  sksReceive   = 0x10,    // 接收到数据包
  sksListen    = 0x20,    // 正在帧听
  sksTimeOut   = 0x80,    // 连接或数据传输超时
  sksNeedSend  = 0x100    // 需要发送
} TSocketState;
//-----------------------------------------------------------------------------
// Socket的工作模式
typedef enum tagSocketMode
{
  skmIdle       = 0,    // 端口未被使用
  skmTCPServer  = 0x01, // 服务器
  skmTCPClient,         // 客户端  用在
  skmUDP,               // UDP
  skmUDPDgram,          // UDP组播模式 
  skmUARTServer,        // 串口的服务器方式
  skmUARTClient         // 串口的客户端方式
} TSocketMode;
//=============================================================================
// 外部对象引用
//-----------------------------------------------------------------------------
class TCOMProtocol;
//=============================================================================
// 公用类定义
//-----------------------------------------------------------------------------
// Socket基类
// 每个连接都对应一个Socket对象
// 在Session中使用本类的方法收发数据
class TCOMSocket
{
  protected:
    uint16_t      m_uwIdent = 0;           // Socket的标识
    uint32_t      m_uState  = 0;           // Socket状态
    TSocketMode   m_smMode  = skmIdle;     // Socket模式
  
    TCOMProtocol *m_pProtocol = nullptr;   // 规约解析器
  
    uint32_t      m_uLocalAddr;
    uint32_t      m_uRemoteAddr;
    uint16_t      m_uwLocalPort;
    uint16_t      m_uwRemotePort;
    uint32_t      m_uInitFRemoteAddr;
    uint16_t      m_uwInitFRemotePort;

  public:         
    uint32_t      m_uBusyCntr = 0;
    uint32_t      m_uNextTick;

  public:
    // 构造
     TCOMSocket() {}
    // 析构
    virtual ~TCOMSocket();
     
    // 释放空间，准备删除本对象
    virtual void Free();
    
    // 读写成员数据
    void StateSet( uint32_t uState)     { m_uState |=  uState; }
    void StateClr( uint32_t uState)     { m_uState &= ~uState; }
    uint32_t StateGet( uint32_t uState) { return (m_uState & uState); }

    virtual  void   SetIdent( uint32_t uIdent )  { m_uwIdent = uIdent; }
    uint16_t GetIdent(void)               { return m_uwIdent;   }
    void     SetMode( TSocketMode sMode ) { m_smMode = sMode;   }

    void          SetProtocol( TCOMProtocol *pProtocol );
    TCOMProtocol* GetProtocol(void)  { return m_pProtocol; }

    uint32_t GetLocalAddr(void)      { return m_uLocalAddr;   }
    uint16_t GetLocalPort(void)      { return m_uwLocalPort;  }
    uint32_t GetRemoteAddr(void)     { return m_uRemoteAddr;  }
    uint16_t GetRemotePort(void)     { return m_uwRemotePort; }

    uint32_t GetInitRemoteAddr(void) { return m_uInitFRemoteAddr;  }
    uint16_t GetInitRemotePort(void) { return m_uwInitFRemotePort; }

    void  SetLocalAddr     (uint32_t uLocalAddr)    
             { m_uLocalAddr = uLocalAddr; }
    void  SetInitRemoteAddr(uint32_t uRemoteIpAddr) 
             { m_uInitFRemoteAddr = uRemoteIpAddr; 
               m_uRemoteAddr = uRemoteIpAddr; }
    void  SetInitRemotePort(uint16_t uRemotePort)   
             { m_uwInitFRemotePort = uRemotePort;
               m_uwRemotePort = uRemotePort;   }
    
    // 完成初始化
    virtual void Init(void);
    
    // 响应通讯事件
    virtual void OnConnect(void);
    virtual void OnDisconnect(void);
    virtual void OnTimeout(void);
    virtual void OnReceive(void);
    virtual void OnReceive(const void* pvMsg, uint32_t uNumBytes = 0 );
    virtual void OnSent(void);

    // 定时事件
    // uTick的单位是1ms
    virtual void OnTick(uint32_t uTick);

    // 读写方法
    // pucBuffer的长度不应小于2048字节
    // 返回：实际读写的字节数
    virtual uint32_t Read ( void *pvBuffer, uint32_t uBufLen ) = 0;
    virtual uint32_t Write( const void* pvBuffer, 
                            uint32_t  uNumBytes,
                            uint32_t  uDestAddr = 0,
                            uint32_t  uDestPort = 0 ) = 0;
      
    virtual int Disonnect(void) { return 0; }
};
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // __SOCKET_BASE_H
