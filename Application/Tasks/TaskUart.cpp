//-----------------------------------------------------------------------------
/*
 File        : TaskUART.c
 Version     : V1.10
 By          : 银网科技

 Description :UART任务

 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "TaskUART.h"

#include "UartChls.h"
#include "SocketUART.h"

#include "EchoSvr.h"
#include "proModbusRTUSlave.h"

#include "RamHeap.h"

#include "DevRegs.h"
#include "DevFixed.h"
#include "DevEvtMgr.h"
#include "DevDebug.h"

#include "usart.h"

#include <cmsis_os.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据申明
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
static TCOMSocketUART *FUartSockets[NUM_UART_MCU] = {nullptr};
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#ifndef size_t
  #define size_t unsigned int
#endif
inline void* operator new(size_t, void* ptr )
{  return ptr; }
//-----------------------------------------------------------------------------
// 创建UARTSocket
static bool UART_BuildSocket(const TUARTConfig* poUART, uint32_t uUIdx)
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( uUIdx >= 2, GFC_ErrParam );
#endif

  FUartSockets[uUIdx] = nullptr;
  
  void* pMem = RAM_Malloc( sizeof(TModbusRTUSlave)  + 4 );
  if( nullptr != pMem )
    {
    TCOMProtocol *pParser = new (pMem)TModbusRTUSlave;
    if( nullptr != pParser )
      {
      pParser->SetEventFilter( NES_UART1 << uUIdx );  // 事件过滤器

      pMem = RAM_Malloc( sizeof(TCOMSocketUART)  + 4 );
      if( nullptr != pMem )
        {
        TCOMSocketUART* pSocket = new (pMem)TCOMSocketUART;
        if( nullptr != pSocket )
          {
          FUartSockets[uUIdx] = pSocket;

          pSocket->SetIdent    ( uUIdx );        // Socket的Ident，见UartChls.h
          pSocket->SetProtocol ( pParser );      // 创建并配置规约解析器
          pSocket->SetLocalAddr( poUART->Addr ); // Slave Address

          pSocket->Init(); 
          pSocket->OnConnect();
          }
#ifdef USE_DEV_ASSERT
        else
          DEV_FAULT( GFC_OutOfMem );
#endif
        }
#ifdef USE_DEV_ASSERT
      else
        DEV_FAULT( GFC_OutOfMem );
#endif
      }
#ifdef USE_DEV_ASSERT
    else
      DEV_FAULT( GFC_OutOfMem );
#endif
    }
#ifdef USE_DEV_ASSERT
  else
    DEV_FAULT( GFC_OutOfMem );
#endif

  return true;
}
//-----------------------------------------------------------------------------
// 释放Socket/Protocol，回收空间
static void UART_DestorySockets(uint32_t uUIdx)
{

  TCOMSocketUART *pSocket = FUartSockets[uUIdx];
  if( nullptr == pSocket )
    return ;

  TCOMProtocol* pProtocol = pSocket->GetProtocol();
  pSocket->SetProtocol( nullptr );

  RAM_Free( pProtocol );
  RAM_Free( pSocket   );

  FUartSockets[uUIdx] = nullptr;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// UART1任务
static void taskUART1(void *argument)
{

  uint32_t uUIdx = (uint32_t)argument;

  const TUARTConfig *poUART = GET_UARTOPT(uUIdx);
  // 创建 Socket
  UART_BuildSocket( poUART, uUIdx );

  UartChl_Config( uUIdx );
  osDelay( 100 );

  // 启动接收
  UartChl_RevNOBStart( uUIdx );

  //
  TUARTCtrlBlock *pcbUART = &GetUARTChl(uUIdx);
  TCOMSocketUART *pSocket = FUartSockets[uUIdx];
  while( true )
    {
    if( nullptr != pcbUART->TxEnPort )
      {
      // 半双工
      int iTryCnt = 100;
      while( USB_SENDING == (pcbUART->usState & USB_SENDING) )
        {
        osDelay(5);
            
        if( USB_CPLSENT == (pcbUART->usState & USB_CPLSENT) ||
            --iTryCnt == 0 )
          {
          // 强制结束发发送
          CloseUartTxEn( uUIdx );

          break;
          }

        SetRSTSrc( RRS_UARTTASK );
        }
      }

    // 需要重新配置硬件
    if( GetSetHW(RSH_UART1 << uUIdx) )
      {
      // 配置硬件
      UartChl_Config( uUIdx );
      osDelay( 100 );

      pSocket->SetLocalAddr( poUART->Addr );

      // 启动接收
      UartChl_RevNOBStart( uUIdx );

      ClrSetHW(RSH_UART1 << uUIdx);
      }

    // 等待接收信号
#if osCMSIS >= 0x20000U
    osSemaphoreAcquire( pcbUART->SimRxMsg, 20 );
#else
    osSemaphoreWait( pcbUART->SimRxMsg, 20 );
#endif

    // 有数据要接收
    if( pSocket->RevNumBytes() > 0 )
      {
      pSocket->OnReceive();
        
      // 报文要发送
      if( 0 != pSocket->StateGet( sksNeedSend ) )
        {
        osDelay( 1 );
        pSocket->OnTick( HAL_GetTick() );
        }

      pcbUART->usIdle = 0;
      }
    else if( pSocket->StateGet( sksReceive ) )
      {
      pcbUART->usIdle++; 

      if( 30000 < pcbUART->usIdle )
        {
        UART_CheckPort( pcbUART );
        pcbUART->usIdle = 0;
        }
      }

    // 控制速度，防止线程锁死
    osDelay(20);

    // 置任务有效
    SetRSTSrc( RRS_UARTTASK );
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 任务
void CreateUARTTask()
{

  UartChl_Init();
//  osDelay( 10 );
  
#if osCMSIS >= 0x20000U
  static const osThreadAttr_t uart1Task_attributes = 
    {
    .name = "UART1Task",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t) osPriorityNormal,
    };
  osThreadNew(taskUART1, (void*)UART1_CHL, &uart1Task_attributes);
#else
  osThreadDef((char*)UART1Task, taskUART1, osPriorityNormal, 0, 1024);
  osThreadCreate(osThread(UART1Task), 0);
#endif
}
//-----------------------------------------------------------------------------
