//-----------------------------------------------------------------------------
/*
 File        : Protocol.h
 Version     : V1.10
 By          : 银网科技

 Description :通讯Protocol基类
              TCOMProtocol是通讯规约的基类，所有通讯规约要从这个类继承。
              有两个方法必须在子类中实现：
                  1. Init
                     Init构造解析器需要的工作环境，如：缓冲区、参数表等
                  2. OnReceive
                     OnReceive中要调用m_pSocket读取端口接收到的报文，
                     并存放到指定缓冲区
        
 Date       : 2023.12.05
 
 2024.1.15
   v2.0
   代码规范
*/
//-----------------------------------------------------------------------------
#ifndef COM_PROTOCOL_BASE_H
#define COM_PROTOCOL_BASE_H

#include <cstdint>

#include "COMDevIntf.h"
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

//=============================================================================
// 外部对象引用
//-----------------------------------------------------------------------------
class TCOMSocket;
//=============================================================================
// 公用类定义
//-----------------------------------------------------------------------------
// Protocol基类
// 每个连接都对应一个Socket对象
// 在Session中使用本类的方法收发数据
class TCOMProtocol : public TCommDevInterface
{
  protected:
     TCOMSocket  *m_pSocket = nullptr;
  
  public:
    // 构造
  TCOMProtocol(void) : TCommDevInterface()  { }
     
  TCOMProtocol( TCOMSocket *pSocket ) : TCommDevInterface()
     { 
     SetSocket( m_pSocket );
     }

    // 析构
    virtual ~TCOMProtocol(void);

    // 读写成员数据
    virtual void SetSocket( TCOMSocket *pSocket );
    TCOMSocket* GetSocket(void)      { return m_pSocket; }

    // 完成初始化
    virtual void Init(void) = 0;
    
    // 释放空间
    virtual void Free(void) = 0;

    // 响应通讯事件
//    virtual void OnConnect(uint32_t uID) {}
    virtual void OnConnect(void)       {}
    virtual void OnDisconnect(void)    {}
    virtual void OnTimeout(void)       {}
    virtual void OnReceive( const void *pvMsg = nullptr, 
                            uint32_t  uNumBytes = 0) = 0;
    virtual void OnSent(void)          {}

    // 定时事件  （子类必须要调用，以确保 遥控和遥写 超时）
    // uTick的单位是1ms
    virtual void OnTick(uint32_t uTick) {}

  protected:
    // 通知Socket发送通讯报文
    virtual uint32_t Write(const void* pvBuffer, uint32_t uNumBytes );  
};
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // COM_PROTOCOL_BASE_H
