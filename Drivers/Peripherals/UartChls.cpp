//-----------------------------------------------------------------------------
/*
 File        : UartChls.h
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f10x
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description : UART信道要用到的结构和方法
        
 Date        : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "UartChls.h"

#include "DevTypes.h"

#include "usart.h"

#include "cmsis_os.h"

#include <string.h>
//===========================================================================
// 局部宏定义
//----------------------------------------------------------------------------

//===========================================================================
// 局部数据结构
//----------------------------------------------------------------------------

//===========================================================================
// 全局数据
//----------------------------------------------------------------------------
// 各UART通道的控制结构
#if NUM_UART_CHLs > 1
 TUARTCtrlBlock  UartCtrls[NUM_UART_CHLs];
#else
 TUARTCtrlBlock  UartCtrl;
#endif
//===========================================================================
// 本地方法
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// 此方法必须在操作系统初始化后使用
void UartChl_OSSim(void)     // 创建OS同步信号
{

  // 初始化MCU内部UART信道的OS同步信号
#if NUM_UART_CHLs > 1
  for( int iIdx = 0; iIdx < NUM_UART_CHLs; iIdx++ )
    {
    osSemaphoreDef(Sim_UART_Rx);
 #if osCMSIS >= 0x20000U
    UartCtrls[iIdx].SimRxMsg = osSemaphoreNew( 4, 1, osSemaphore(Sim_UART_Rx) );
#else
    UartCtrls[iIdx].SimRxMsg = osSemaphoreCreate(osSemaphore(Sim_UART_Rx), 1);
#endif
    }
#else
  osSemaphoreDef(Sim_UART_Rx);
 #if osCMSIS >= 0x20000U
    UartCtrl.SimRxMsg = osSemaphoreNew( 4, 1, osSemaphore(Sim_UART_Rx) );
 #else
    UartCtrl.SimRxMsg = osSemaphoreCreate(osSemaphore(Sim_UART_Rx), 1);
 #endif
#endif    
}
//===========================================================================
// 全局方法
//----------------------------------------------------------------------------
// 初始化各UART信道
void UartChl_Init(void)
{

#if NUM_UART_CHLs > 1
  memset( &UartCtrls, 0, sizeof(UartCtrls) );
  
  // 初始化MCU内部UART信道的UartCtrls
  for( int iIdx = 0; iIdx < NUM_UART_CHLs; iIdx++ )
    {
    UartCtrls[iIdx].usState = USB_REVING;
    }
#else
  memset( &UartCtrl, 0, sizeof(UartCtrl) );
  
  // 初始化MCU内部UART信道的UartCtrls
  UartCtrl.usState = USB_REVING;
#endif    
    
  // 创建同步信号  
  UartChl_OSSim();   
}
//----------------------------------------------------------------------------
// UART配置变化 
void UartChl_Config(uint32_t uChlIdx)
{

  if( UARTCHL_MCU0 == uChlIdx )
    // 配置内部通道
    MX_UART1_Init();

#if NUM_UART_MCU > 1
  if( UARTCHL_MCU1 == uChlIdx )
    MX_UART3_Init();
#endif
}
//----------------------------------------------------------------------------
// 启动UART非阻塞接收
void UartChl_RevNOBStart(uint32_t uChlIdx)
{

#if NUM_UART_CHLs > 1
  for( int iIdx = 0; iIdx < NUM_UART_CHLs; iIdx++ )
    {
    UartCtrls[iIdx].usState    = USB_REVING;
    UartCtrls[iIdx].uwRxBufIdx = 0;
    UartCtrls[iIdx].uwRevACnt  = 0;
    UartCtrls[iIdx].uwRevBCnt  = 0;

    __HAL_UART_ENABLE( &UartCtrls[iIdx].Uart );  
    UART_Listen( &UartCtrls[iIdx] );

    __HAL_UART_ENABLE_IT(&UartCtrls[iIdx].Uart, UART_IT_IDLE);
    }
#else
  (void)uChlIdx;
  
  UartCtrl.usState    = USB_REVING;
  UartCtrl.uwRxBufIdx = 0;
  UartCtrl.uwRevACnt  = 0;
  UartCtrl.uwRevBCnt  = 0;

  __HAL_UART_ENABLE( &UartCtrl.Uart );
  UART_Listen( &UartCtrl );

  __HAL_UART_ENABLE_IT(&UartCtrl.Uart, UART_IT_IDLE);
#endif
}
//----------------------------------------------------------------------------
// UART信道接收到信息
HAL_StatusTypeDef UartChl_Received(uint32_t uChlIdx)
{

  HAL_StatusTypeDef sRes;
  if( uChlIdx < NUM_UART_CHLs )
    {
    // 发送信号
    if( nullptr != GetUARTChl(uChlIdx).SimRxMsg )
      {
      osSemaphoreRelease( GetUARTChl(uChlIdx).SimRxMsg );
      sRes = HAL_OK;
      }
    else
      sRes = HAL_ERROR;
    }
  else
    sRes = HAL_ERROR;

  return sRes;
}
//----------------------------------------------------------------------------
// 关闭UART通道发送信号
HAL_StatusTypeDef CloseUartTxEn(uint32_t uChlIdx)
{

  HAL_StatusTypeDef sRes = HAL_ERROR;
  if( uChlIdx < NUM_UART_CHLs )
    {
    TUARTCtrlBlock *pUartCtrl = &GetUARTChl( uChlIdx );

    // 等待发送结束
    if( USB_CPLSENT == (pUartCtrl->usState & USB_CPLSENT) )
      {
      UART_HandleTypeDef *huart = &(pUartCtrl->Uart);

      // 串口发送完？
      if( __HAL_UART_GET_FLAG( huart, UART_FLAG_TC ) != RESET )
        {
        osDelay(5);

        // 重新开始接收
        UART_Listen( pUartCtrl );
  
        // 清标志
        pUartCtrl->usState &= ~(USB_CPLSENT | USB_SENDING);

        // TOGGLE_DBUG; // debug only
        sRes = HAL_OK;
        }
      }
    else
      sRes = HAL_OK;
    }

  return sRes;
}
//----------------------------------------------------------------------------
// 向UART信道发送报文
HAL_StatusTypeDef UartChl_Send(   uint32_t  uChlIdx, 
                                const void *pvBuff, 
                                  uint32_t  uNumBytes )
{

  HAL_StatusTypeDef sRes = UART_Send( uChlIdx, pvBuff, uNumBytes );
  TUARTCtrlBlock *pUartCtrl = &GetUARTChl(uChlIdx);
  if( HAL_OK == sRes )
    {
    pUartCtrl->usDMABusyCntr = 0;
    }
  else if( 100 < ++(pUartCtrl->usDMABusyCntr) &&
           nullptr != pUartCtrl->Uart.hdmatx )
    {
    // 结束发送
    HAL_DMA_Abort( pUartCtrl->Uart.hdmatx );
      
    // 开始接收
    UART_Listen( pUartCtrl );

    pUartCtrl->usDMABusyCntr = 0;
    }
  
  return sRes;
}
//----------------------------------------------------------------------------
