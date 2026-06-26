//-----------------------------------------------------------------------------
/*
 File        : proModbusRTUSlave.h
 Version     : V1.10
 By          : 银网科技

 Description :定义Modbus RTU Slave协议对象

 Date       : 2024.1.18
*/
//-----------------------------------------------------------------------------
#ifndef  COM_PROMODBUS_RTU_SLAVE_H_
#define  COM_PROMODBUS_RTU_SLAVE_H_

#include "Protocol.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 全局对象引用
//-----------------------------------------------------------------------------
class TCOMBuffer;
//=============================================================================
// 全局对象
//-----------------------------------------------------------------------------
class TModbusRTUSlave : public TCOMProtocol
{
  protected:
    uint8_t     *m_pucTxBuf   = nullptr;
    TCOMBuffer  *m_pRxBuffer  = nullptr;
  
  public:
    TModbusRTUSlave() : TCOMProtocol() {}
    TModbusRTUSlave( TCOMSocket *pSocket ) : TCOMProtocol( pSocket ) {}
      
    virtual void OnReceive( const void* pvMsg = 0, uint32_t uNumBytes = 0);
  
    // 定时事件
    // uTick的单位是1ms
    virtual void OnTick(uint32_t uTick);

  protected:
    // 完成初始化
    virtual void Init(void);
    
    // 释放空间
    virtual void Free(void);
    
  private:
    // 解析报文
    void parseMessage(void);
  
    // 分配报文解析
    void dispatch(void);

    // 发送报文
    // 输入：
    //   uLength: 报文长度，不含校验域
    void sendMessage(uint32_t uLength);

    //------------- 执行规约中的各功能
    // 读线圈寄存器  0x01
    bool coilRead(void);
    // 写单个线圈寄存器  0x05
    bool coilWrite(void);
    // 写多个线圈寄存器  0x0F
    bool coilMutipleWrite(void);
  
    // 读离散寄存器 0x02
    bool discreteInputsRead(void);
    
    // 读保持寄存器 0x03
    bool holdingRead(void);
    // 写保持寄存器 0x06
    bool holdingWrite(void);
    // 读保持寄存器 0x10
    bool holdingMutipleWrite(void);
    
    // 读输入寄存器 0x04
    bool inputRead(void);

    // 读SOE 0x0C
    bool eventlogRead(void);
};
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // COM_PROMODBUS_RTU_SLAVE_H_
