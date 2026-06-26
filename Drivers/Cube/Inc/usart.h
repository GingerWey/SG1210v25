//-----------------------------------------------------------------------------
/*
 File        : uart.h
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f10x
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description : UART驱动
        
 Date        : 2023.12.05
  */
//-----------------------------------------------------------------------------
#ifndef DRV_USART_H
#define DRV_USART_H

#include "DevTypes.h"
#include "UartChls.h"

//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------------
// 初始化USART1
void MX_UART1_Init(void);

// USART中断响应
uint32_t UART_IRQHandler(uint8_t uChl);

// 开始非阻塞接收
uint32_t UART_Listen(TUARTCtrlBlock *pUart);

// 发送一个缓冲区
HAL_StatusTypeDef UART_Send(   uint8_t  uChl, 
                           const  void *pvBuff, 
                              uint32_t  uNumBytes );

// 检查Uart端口状态
// 当接收超时，检查端口状态，并复位
void UART_CheckPort(TUARTCtrlBlock *pUart);

// 切换全部UART通道为接收状态
void CloseAllUartTxEn(void);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif /* __USART_H__ */

