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
#ifndef DRV_UARTCHLS_H
#define DRV_UARTCHLS_H

#include "stm32f4xx_hal.h"

#include "cmsis_os.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
 extern "C" {
#endif
//===========================================================================
// 全局常量
//----------------------------------------------------------------------------
//#define USES_CloseTxEnTimer
//===========================================================================
// 全局宏定义
//----------------------------------------------------------------------------
// UART通道
// 用于应用程序访问底层驱动
// 序号与UartCtrls对应
// MCU上的UART
#define  UARTCHL_MCU0   0

// UART硬件通道编号
// 用于驱动层
#define  UART1_CHL        UARTCHL_MCU0   // RS485

// UART状态
#define  USB_IDLE          0x0000        // 空闲
#define  USB_SENDING       0x0001        // 正在发送中
#define  USB_REVING        0x0002        // 正在接收
#define  USB_CPLSENT       0x0004        // 等待结束
#define  USB_PAUSED        0x8000        // 通讯中断
///----------------------------------------------------------------------------
// 缓冲区尺寸
#define  SIZE_UARTBUFF        512
//----------------------------------------------------------------------------
// UART通道数量
#define  NUM_UART_CHLs          1        // 1 RS232 / RS485 / RS422
#define  NUM_UART_MCU           1        // 使用MCU内部UART的个数
//===========================================================================
// 全局数据结构
//----------------------------------------------------------------------------
typedef struct tagUARTCtrlBlock
{
  
  UART_HandleTypeDef   Uart;
  GPIO_TypeDef        *TxEnPort;
  uint16_t             TxEnPin;

  DMA_HandleTypeDef    rxDMA;
  DMA_HandleTypeDef    txDMA;

  osSemaphoreId        SimRxMsg;

  uint16_t             usState;  
  uint16_t             usIdle;
  uint16_t             usDMABusyCntr;

  uint16_t             uwRxBufIdx;    // 当前的接收缓冲区 0-A 1-B
                       
  uint16_t             uwRevACnt;
  uint8_t              ucRxBufA[SIZE_UARTBUFF];

  uint16_t             uwRevBCnt;
  uint8_t              ucRxBufB[SIZE_UARTBUFF];
                      
  uint16_t             uwOrderA; 
  uint16_t             uwOrderB;
} TUARTCtrlBlock;
//===========================================================================
// 全局数据
//----------------------------------------------------------------------------
// 各UART通道的控制结构
#if NUM_UART_CHLs > 1
  extern TUARTCtrlBlock  UartCtrls[NUM_UART_CHLs];
#else
  extern TUARTCtrlBlock  UartCtrl;
#endif
//----------------------------------------------------------------------------
#if NUM_UART_CHLs > 1
  #define  GetUARTChl(x)  (UartCtrls[x])
#else
  #define  GetUARTChl(x)  (UartCtrl)
#endif
//===========================================================================
// 全局方法
//----------------------------------------------------------------------------
// 初始化各UART信道
// 此方法必须在DevOpts装载后方可使用
void UartChl_Init(void);

// UART配置变化
// uChlIdx: 逻辑通道 UARTCHL_MCUx
void UartChl_Config(uint32_t uChlIdx);
//----------------------------------------------------------------------------
// UART信道接收到信息
HAL_StatusTypeDef UartChl_Received(uint32_t uChlIdx);
//----------------------------------------------------------------------------
// 关闭UART通道发送信号
HAL_StatusTypeDef CloseUartTxEn(uint32_t uChlIdx);
//----------------------------------------------------------------------------
// 启动UART非阻塞接收
void UartChl_RevNOBStart(uint32_t uChlIdx);

// 向UART信道发送报文
HAL_StatusTypeDef UartChl_Send(   uint32_t  uChlIdx, 
                                const void *pvBuff, 
                                  uint32_t  uNumBytes);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // __UARTCHLS_H
