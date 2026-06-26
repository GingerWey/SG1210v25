//-----------------------------------------------------------------------------
/*
 File        : InspSvr.h
 Version     : V1.10
 By          : 银网科技

 Description :定义Inspector服务器协议对象

 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef  __INSP_SVR_H__
#define  __INSP_SVR_H__

#include "DevTypes.h"

#include "Protocol.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
#if AOS_ENABLE > 0
 extern TInspectorAOS AOSBuffer;
#endif
//=============================================================================
// 全局对象引用
//-----------------------------------------------------------------------------

//=============================================================================
// 全局对象
//-----------------------------------------------------------------------------
class TInspectorServer : public TCOMProtocol
{
  protected:
    uint16_t  m_uwRxCntr;
    uint8_t  *m_pucRxBuf, *m_pucTxBuf;
  
    uint32_t  m_uToken;
  
  public:
    TInspectorServer() : TCOMProtocol() 
      { 
      m_uwRxCntr  = 0;
      m_pucRxBuf = nullptr;
      m_pucTxBuf = nullptr;
      }
    TInspectorServer( TCOMSocket *pSocket ) : TCOMProtocol( pSocket ) 
      {  
      m_uwRxCntr  = 0;
      m_pucRxBuf = nullptr;
      m_pucTxBuf = nullptr; 
      }
    
    // 释放空间
    virtual void Free(void);
      
    virtual void OnReceive(void* pvMsg = 0, uint32_t uNumBytes = 0);
  
    // 定时事件
    // uTick的单位是1ms
    virtual void OnTick(uint32_t uTick);

  protected:
    // 完成初始化
    virtual void Init(void);
  
  private:
    // 解析报文
    void Parser(const uint8_t *pucMsg, uint32_t uNumBytes);
  
    // 发送报文
    void SendMsg( uint8_t ucFunc, uint16_t uwInfoLen );
  
    // 发送 ISP_DEVINFO  
    void Send_ISP_DEVINFO(void);
    // 发送 ISP_DEVRTRES  
    void Send_ISP_DEVRTRES(void);
    // 发送 ISP_REGREAD  
    void Send_ISP_REGREAD(uint32_t uRegNum, uint16_t uwCount);
    // 发送 ISP_REGPROP  
    void Send_ISP_REGPROP(uint32_t uRegNum);
  
    // ---- 访问录波数据 -----
    // 录波器摘要
    void Send_ISP_WL_DIGEST(void);
    // 录波器通道描述
    void Send_ISP_WL_CHLINFO( uint16_t uwChlBeg );
    // 读录波记录                              
    void Send_ISP_WL_RECINFO( uint16_t uwRecBeg );
    // 读录波录波数据
    void Send_ISP_WL_RDDATA( uint16_t uwRecNo, 
                             uint16_t uwChlNo, 
                             uint32_t uPosition );
    // 读录波记录                              
    void Send_ISP_WL_CLEAR( uint16_t len );

    // 写寄存器
    uint32_t WriteRegs( uint32_t uToken,  uint32_t uRegBeg, 
                        uint32_t uRegNum, uint16_t* puwData );

    // 执行
    uint32_t WriteExec( uint32_t uToken );
  
#if AOS_ENABLE > 0
    // 处理AOS任务
    void Send_ISP_SETAOS(const uint8_t* pucBuf, uint16_t uwLen);
    void Send_ISP_CLRAOS(void);
    void Send_ISP_AOSDATA(void);
#endif
};
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // __INSP_SVR_H__
