//-----------------------------------------------------------------------------
/*
 File        : uart.c
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f10x
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description : UART驱动

 Date        : 2023.12.05
 
 2024.1.16
   v2.0 Wey.
   解决在SG1210v1.20硬件上DMA发送不全的Bug
*/
//-----------------------------------------------------------------------------
#include "usart.h"

#include "gpio.h"
#include "dma.h"

#include "DevRegs.h"
#include "DevDebug.h"
//=============================================================================
// 本地宏定义
//-----------------------------------------------------------------------------
#ifdef USART1_TxEn_Pin
 #define  USART1_TEn_Port    USART1_TxEn_Port
 #define  USART1_TEn_Pin     USART1_TxEn_Pin 
#else
 #define  USART1_TEn_Port    0
 #define  USART1_TEn_Pin     0
#endif

// TxEn高电平，发送使能
#define  UARTx_Tx_Enabled(x)      SetPinToHigh( (x)->TxEnPort, (x)->TxEnPin )
// TxEn低电平，接收使能           
#define  UARTx_Rx_Enabled(x)      SetPinToLow( (x)->TxEnPort, (x)->TxEnPin )
                                  
// 判断是否发送状态               
#define  UARTx_Is_TxState(x)      ((x)->TxEnPort->ODR & (x)->TxEnPin)
// 判断是否半双工                 
#define  UARTx_Is_HalfDuplex(x)   (nullptr != ((x)->TxEnPort))
//=============================================================================
// 本地常量
//-----------------------------------------------------------------------------
// 按1/4800存贮的波特率
constexpr uint8_t cuBaudrates[] =
{
  1, 2, 3, 4, 24
};
#define Num_cuBaudrates    sizeof(cuBaudrates) / sizeof(cuBaudrates[0])
//=============================================================================
// 全局变量
//-----------------------------------------------------------------------------
void UartDmaErrorCallback(struct __DMA_HandleTypeDef * hdma);
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// USART1 init function
void MX_UART1_Init(void)
{

  TUARTCtrlBlock  *pUartCtrl = &GetUARTChl(UART1_CHL);
  
  __HAL_UART_DISABLE( &(pUartCtrl->Uart) );

  const TUARTConfig *puoUart = GET_UARTOPT(UART1_CHL);
  
  pUartCtrl->Uart.Instance = USART1;
  
  // Baudrate
  if( puoUart->Baudrate < Num_cuBaudrates )
    pUartCtrl->Uart.Init.BaudRate   = cuBaudrates[puoUart->Baudrate] * 4800;
  else
    pUartCtrl->Uart.Init.BaudRate   = 9600;

  // Parity
  switch( (puoUart->Parity) )
    {
    case 1:
      pUartCtrl->Uart.Init.Parity     = UART_PARITY_EVEN;
      pUartCtrl->Uart.Init.WordLength = UART_WORDLENGTH_9B;
      break;
    case 2:
      pUartCtrl->Uart.Init.Parity     = UART_PARITY_ODD;
      pUartCtrl->Uart.Init.WordLength = UART_WORDLENGTH_9B;
      break;
    default:
      pUartCtrl->Uart.Init.Parity     = UART_PARITY_NONE;
      pUartCtrl->Uart.Init.WordLength = UART_WORDLENGTH_8B;
      break;
    }

  pUartCtrl->Uart.Init.StopBits     = UART_STOPBITS_1;
  pUartCtrl->Uart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  pUartCtrl->Uart.Init.Mode         = UART_MODE_TX_RX;
  pUartCtrl->Uart.Init.OverSampling = UART_OVERSAMPLING_16;
    
  if (HAL_UART_Init(&(pUartCtrl->Uart)) != HAL_OK)
    {
    SetHWFault( RHF_UART_ERR );
    }

  pUartCtrl->Uart.hdmarx = &(GetUARTChl(UART1_CHL).rxDMA);
  pUartCtrl->Uart.hdmatx = &(GetUARTChl(UART1_CHL).txDMA);
  
  // 只有485模块使用半双工
  pUartCtrl->TxEnPort = USART1_TxEn_Port;
  pUartCtrl->TxEnPin  = USART1_TxEn_Pin;
}
//-----------------------------------------------------------------------------
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
    {
    // USART1 clock enable
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
      PA9  ------> USART1_TX     v1.20
      PA10 ------> USART1_RX 
    */
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;  // Wey. 下拉降低了误码
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    GPIO_InitStruct.Pin   = USART1_TXD_Pin | USART1_RXD_Pin;
    HAL_GPIO_Init(USART1_RXD_Port, &GPIO_InitStruct);

    DMA_HandleTypeDef *hdma = &(GetUARTChl(UART1_CHL).rxDMA);
    hdma->Instance          = DMA2_Stream2;
    hdma->Init.Channel      = DMA_CHANNEL_4;
    hdma->Init.Direction            = DMA_PERIPH_TO_MEMORY;
    hdma->Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma->Init.MemInc               = DMA_MINC_ENABLE;
    hdma->Init.Mode                 = DMA_NORMAL;
    hdma->Init.Priority             = DMA_PRIORITY_MEDIUM;
    hdma->Init.MemBurst             = DMA_MBURST_SINGLE;
    hdma->Init.FIFOMode             = DMA_FIFOMODE_DISABLE;
    hdma->Init.FIFOThreshold        = DMA_FIFO_THRESHOLD_FULL;
    hdma->Init.PeriphBurst          = DMA_PBURST_SINGLE;
    hdma->Init.PeriphDataAlignment  = DMA_PDATAALIGN_BYTE;
    hdma->Init.MemDataAlignment     = DMA_MDATAALIGN_BYTE;
    hdma->XferErrorCallback         = UartDmaErrorCallback;
    HAL_DMA_Init( hdma );

    __HAL_LINKDMA(uartHandle, hdmarx, *hdma);

    hdma = &(GetUARTChl(UART1_CHL).txDMA);
    hdma->Instance          = DMA2_Stream7;
    hdma->Init.Channel      = DMA_CHANNEL_4;
    hdma->Init.Direction            = DMA_MEMORY_TO_PERIPH;
    hdma->Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma->Init.MemInc               = DMA_MINC_ENABLE;
    hdma->Init.Mode                 = DMA_NORMAL;
    hdma->Init.Priority             = DMA_PRIORITY_MEDIUM;
    hdma->Init.MemBurst             = DMA_MBURST_SINGLE;
    hdma->Init.FIFOMode             = DMA_FIFOMODE_DISABLE;
    hdma->Init.FIFOThreshold        = DMA_FIFO_THRESHOLD_FULL;
    hdma->Init.PeriphBurst          = DMA_PBURST_SINGLE;
    hdma->Init.PeriphDataAlignment  = DMA_PDATAALIGN_BYTE;
    hdma->Init.MemDataAlignment     = DMA_MDATAALIGN_BYTE;
    hdma->XferErrorCallback         = UartDmaErrorCallback;
    HAL_DMA_Init( hdma );

    __HAL_LINKDMA(uartHandle, hdmatx, *hdma);

    // Peripheral interrupt init
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}
//-----------------------------------------------------------------------------
// 
uint32_t UART_Listen(TUARTCtrlBlock *pUartCtrl)
{

  UART_HandleTypeDef *huart   = &(pUartCtrl->Uart);
  
  // 半双工？  
  if( 0 != UARTx_Is_HalfDuplex( pUartCtrl ) )
    {  
    // 切换收发器到接收状态
    UARTx_Rx_Enabled( pUartCtrl );

    // 停止接收和发送
    HAL_UART_Abort( huart );

    pUartCtrl->usState = USB_REVING;
    }                           
  else
    {
    // 关闭接收
    HAL_UART_AbortReceive( huart );

    pUartCtrl->usState |= USB_REVING;
    }
   
  // 当前要使用的接收缓冲区
  uint8_t *pucRxBuf = (pUartCtrl->uwRxBufIdx == 0)? 
                       pUartCtrl->ucRxBufA : pUartCtrl->ucRxBufB;

  // 优先采用DMA
  if( nullptr != huart->hdmarx )
    {
    // 重新开始DMA接收
    //HAL_UARTEx_ReceiveToIdle_DMA( huart,
    HAL_UART_Receive_DMA( &(pUartCtrl->Uart),
                                  pucRxBuf,
                                  SIZE_UARTBUFF );
    }
  else // 没有DMA，用中断
    {
    // 开始新的中断接收
    //HAL_UARTEx_ReceiveToIdle_IT( huart,
    HAL_UART_Receive_IT( &(pUartCtrl->Uart),
                          pucRxBuf,
                          SIZE_UARTBUFF );
    }

   return __HAL_UART_ENABLE_IT( huart, UART_IT_IDLE);
}
//-----------------------------------------------------------------------------
// 从缓冲区中接收
uint32_t UART_Received( uint32_t uChl )
{

  uint32_t uRevCnt;
  TUARTCtrlBlock *pUartCtrl = &GetUARTChl(uChl);

  if( nullptr != pUartCtrl->Uart.hdmarx )
    uRevCnt = SIZE_UARTBUFF - __HAL_DMA_GET_COUNTER( pUartCtrl->Uart.hdmarx );
  else
    uRevCnt = SIZE_UARTBUFF - pUartCtrl->Uart.RxXferCount;  

  if( 0 < uRevCnt )
    {
    // 切换接收缓冲      
    if( 0 == pUartCtrl->uwRxBufIdx )
      {
      pUartCtrl->uwRevACnt = uRevCnt;

      pUartCtrl->uwOrderB  = pUartCtrl->uwOrderA + 1;
      pUartCtrl->uwRxBufIdx  = 1;
      }
    else
      {
      pUartCtrl->uwRevBCnt = uRevCnt;
        
      pUartCtrl->uwOrderA  = pUartCtrl->uwOrderB + 1;
      pUartCtrl->uwRxBufIdx  = 0;
      }

    // 发报文收到通知
    UartChl_Received( uChl );
    }

  return uRevCnt;
}
//-----------------------------------------------------------------------------
// 中断响应
uint32_t UART_IRQHandler(uint8_t uChl)
{

  TUARTCtrlBlock     *pUartCtrl = &GetUARTChl(uChl);
  UART_HandleTypeDef *huart     = &(pUartCtrl->Uart); 

  // 总线空闲
  __IO uint32_t uRes = 0;

  // UART in mode Transmitter end ----------------
  if( (__HAL_UART_GET_FLAG( huart, UART_FLAG_IDLE ) != RESET) && 
      (__HAL_UART_GET_IT_SOURCE( huart, UART_IT_IDLE ) != RESET) )
    {
    __HAL_UART_CLEAR_IDLEFLAG( huart );

    // Wey. 2024/01/16
    // 硬件上有干扰时，会错误关闭发送
    if( 0 != UARTx_Is_HalfDuplex( pUartCtrl ) )
      {
      // 半双工
      // 仅当接收状态下，处理接收的数据
      if( USB_REVING == (pUartCtrl->usState & USB_REVING) )
         UART_Received( uChl ); // Wey. 2017.10.16
      
      // 发送未结束前，不启动接收
      if( 0 == (pUartCtrl->usState & USB_SENDING) )
        // 重新开始接收
        UART_Listen( pUartCtrl );
      }
    else
      {
      // 全双工
      UART_Received( uChl ); // Wey. 2017.10.16

      // 重新开始接收
      UART_Listen( pUartCtrl );
      }

    // 清PE、FE、NE、ORE、IDLE标志位
    // PE (Parity error), FE (Framing error), NE (Noise error), ORE
    // (Overrun error) and IDLE (Idle line detected) flags are cleared
    // by software sequence: a read operation to USART_SR register followed
    // by a read operation to USART_DR register.
    uRes = huart->Instance->SR;
    uRes = huart->Instance->DR;
      
    uRes = UART_IT_IDLE;
    }  

  HAL_UART_IRQHandler( huart );
    
  return uRes;
}
//-----------------------------------------------------------------------------
 void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

#if NUM_UART_MCU > 1  
  for( uint32_t uIdx = 0; uIdx < NUM_UART_MCU; uIdx++ )
    {
    TUARTCtrlBlock *pUartCtrl = &GetUARTChl(uIdx);
#else
    {
    TUARTCtrlBlock *pUartCtrl = &GetUARTChl(0);
#endif
    if( pUartCtrl->Uart.Instance == huart->Instance )
      { 
      int iRstTx = 0;        
      switch( huart->ErrorCode )
        {
        case HAL_UART_ERROR_PE:
          __HAL_UART_CLEAR_PEFLAG( huart );
          iRstTx = 1;

        case HAL_UART_ERROR_NE:
          __HAL_UART_CLEAR_NEFLAG( huart );
          break;

        case HAL_UART_ERROR_FE:
          {
          //while( 0 != (huart->Instance->SR & UART_FLAG_RXNE) )
            {
            __IO uint8_t ucChar = huart->Instance->DR;  // 清RXNE 
            __HAL_UART_CLEAR_FEFLAG( huart );
            }
          iRstTx = 1;
          break;
          }

        case HAL_UART_ERROR_ORE:
          {
          __IO uint8_t ucChar = huart->Instance->DR;  // 清RXNE 
          __HAL_UART_CLEAR_OREFLAG( huart );
          break;
          }

        case HAL_UART_ERROR_DMA:
          {
          if( nullptr != huart->hdmatx && 
              HAL_DMA_ERROR_NONE != pUartCtrl->Uart.hdmatx->ErrorCode )
             UartDmaErrorCallback( pUartCtrl->Uart.hdmatx ); 
          
          if( nullptr != huart->hdmarx && 
              HAL_DMA_ERROR_NONE != pUartCtrl->Uart.hdmarx->ErrorCode )
             UartDmaErrorCallback( pUartCtrl->Uart.hdmarx ); 

          break;
          }
        }

      if( iRstTx > 0 )
        {
#if NUM_UART_MCU > 1  
        if( UARTCHL_MCU0 == uIdx )
#endif
          {
 #ifdef UIB1_RST_ACTIVE
          // 发同步信号
          UIB1_RST_ACTIVE;
       
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();

//          // 重新初始化
//          //HAL_UART_Init( huart );
//          HAL_UART_Abort( huart ); 
//          HAL_UART_MspInit( huart );  

          // 信道1复位
          UIB1_RST_DEACTIVE;

          // 要求复位
          //SetTodoTask(RTI_RESET_UIB1);
 #endif
          }

#if NUM_UART_MCU > 1  
        else if( UARTCHL_MCU1 == uIdx )
          {
 #ifdef UIB2_RST_ACTIVE
          // 发同步信号
          UIB2_RST_ACTIVE;

          // 要求复位
          //SetTodoTask(RTI_RESET_UIB2);
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();
          __NOP();

//          // 重新初始化
//          //HAL_UART_Init( huart );
//          HAL_UART_Abort( huart ); 
//          HAL_UART_MspInit( huart );  

          // 信道1复位
          UIB2_RST_DEACTIVE;
 #endif    
          }
#endif // NUM_UART_MCU > 1
        }

      huart->ErrorCode = HAL_UART_ERROR_NONE;

//#if !(defined(UIB1_RST_ACTIVE) || defined(UIB2_RST_ACTIVE))
      // 开始帧听
      UART_Listen( pUartCtrl );
//#endif

#if NUM_UART_MCU > 1  
      break;
#endif
      }
    } 
}
//-----------------------------------------------------------------------------
void UartDmaErrorCallback(struct __DMA_HandleTypeDef * hdma)
{

//  TUARTCtrlBlock *pUartCtrl = &GetUARTChl(UART1_CHL);
//  if( hdma == &(pUartCtrl->txDMA) )
//    {
//    pUartCtrl->Uart.Instance->CR3 &= ~USART_CR3_DMAT;  // 停止DMA发送
//      
//    // DMA2_Stream7_IRQn
//    switch( hdma->ErrorCode )
//      {
//      case HAL_DMA_ERROR_TE:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_TEIF3_7);
//        break;
//      case HAL_DMA_ERROR_FE:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_FEIF3_7);
//        break;
//      case HAL_DMA_ERROR_DME:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_DMEIF3_7);
//        break;
//      }

//    HAL_UART_TxCpltCallback( &pUartCtrl->Uart );
//    }
//  else if( hdma == &(pUartCtrl->rxDMA) )
//    {      
//    pUartCtrl = &GetUARTChl(UART1_CHL);
//    pUartCtrl->Uart.Instance->CR3 &= ~USART_CR3_DMAR;  // 停止DMA接收
//      
//    // DMA2_Stream2_IRQn
//    switch( hdma->ErrorCode )
//      {
//      case HAL_DMA_ERROR_TE:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_TEIF2_6);
//        break;
//      case HAL_DMA_ERROR_FE:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_FEIF2_6);
//        break;
//      case HAL_DMA_ERROR_DME:
//        __HAL_DMA_CLEAR_FLAG( hdma, DMA_FLAG_DMEIF2_6);
//        break;
//      }
//      
//    HAL_UART_RxCpltCallback( &pUartCtrl->Uart );
//    }

//  HAL_DMA_Abort( hdma );
//  hdma->ErrorCode = HAL_DMA_ERROR_NONE;
}   
//-----------------------------------------------------------------------------
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{

//#if NUM_UART_MCU > 1  
//  for( uint32_t uIdx = 0; uIdx < NUM_UART_MCU; uIdx++ )
//    {
//    TUARTCtrlBlock *pUartCtrl = &GetUARTChl(UARTCHL_MCU0 + uIdx);
//#else
//    TUARTCtrlBlock *pUartCtrl = &GetUARTChl( UARTCHL_MCU0 );
//#endif
//    if( pUartCtrl->Uart.Instance == huart->Instance )
//      {
//      if( 0 == UARTx_Is_HalfDuplex( pUartCtrl ) )
//        // 全双工, 清[发送状态]
//        pUartCtrl->usState = USB_REVING;
//      else
//        // 等待最后一字节发送结束
//        pUartCtrl->usState |= USB_CPLSENT;

//#if NUM_UART_MCU > 1
//      break;
//      }
//#endif
//    }
//}
////-----------------------------------------------------------------------------
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{

//#if NUM_UART_MCU > 1  
//  for( uint32_t uIdx = 0; uIdx < NUM_UART_MCU; uIdx++ )
//    {
//    if( GetUARTChl(uIdx).Uart.Instance == huart->Instance )
//      {
//      UART_Received( uIdx );    
//  
//      TUARTCtrlBlock *pUartCtrl = &GetUARTChl(uIdx);

//      // 重新开始接收
//       uint8_t *pucRxBuf = (pUartCtrl->uwRxBufIdx == 0)? 
//                           pUartCtrl->ucRxBufA : pUartCtrl->ucRxBufB;

//      if( nullptr != pUartCtrl->Uart.hdmarx ) 
//        // 重新开始DMA接收
//        HAL_UARTEx_ReceiveToIdle_DMA( &(pUartCtrl->Uart),
//                                       pucRxBuf,
//                                       SIZE_UARTBUFF );
//      else
//        // 开始新的中断接收
//        HAL_UARTEx_ReceiveToIdle_IT( &(pUartCtrl->Uart),
//                                      pucRxBuf,
//                                      SIZE_UARTBUFF );
//      break;
//      }
//    }
//#else
//  TUARTCtrlBlock *pUartCtrl = &GetUARTChl(UARTCHL_MCU0);
//  if( nullptr != pUartCtrl )
//    {
//    // 重新开始接收
//     uint8_t *pucRxBuf = (pUartCtrl->uwRxBufIdx == 0)? 
//                         pUartCtrl->ucRxBufA : pUartCtrl->ucRxBufB;
//    if( nullptr != pUartCtrl->Uart.hdmarx )
//      // 重新开始DMA接收
//      //HAL_UARTEx_ReceiveToIdle_DMA( &(pUartCtrl->Uart),
//      HAL_UART_Receive_DMA( &(pUartCtrl->Uart),
//                                    pucRxBuf,
//                                    SIZE_UARTBUFF );
//    else
//      // 开始新的中断接收
//      //HAL_UARTEx_ReceiveToIdle_IT( &(pUartCtrl->Uart),
//      HAL_UART_Receive_IT( &(pUartCtrl->Uart),
//                                   pucRxBuf,
//                                   SIZE_UARTBUFF );
//    }
//#endif  
//}
//-----------------------------------------------------------------------------
// 
HAL_StatusTypeDef UART_Send( uint8_t  uChl, 
                          const void *pvBuff, 
                            uint32_t  uNumBytes )
{

  HAL_StatusTypeDef sRes;
  TUARTCtrlBlock   *pUartCtrl = &GetUARTChl(uChl);
  UART_HandleTypeDef *huart   = &(pUartCtrl->Uart); 
  
  if( nullptr == pUartCtrl || nullptr == pvBuff || 0 == uNumBytes )
    return HAL_ERROR;
  
  if( USB_SENDING == (pUartCtrl->usState & USB_SENDING) )
    return HAL_BUSY;

  if( 0 != UARTx_Is_HalfDuplex( pUartCtrl ) )  // 半双工状态?
    {
    // 停止接收和发送
    HAL_UART_Abort( huart );
      
    // 高电平，使能发送
    UARTx_Tx_Enabled( pUartCtrl );
        
    osDelay(1); 

    pUartCtrl->usState = USB_SENDING;
    }
  else
    {
    // 只停止发送
    HAL_UART_AbortTransmit( huart );
      
    // 置UART正在发送
    pUartCtrl->usState |= USB_SENDING;
    }

  // 优先采用DMA发送
  if( nullptr != huart->hdmatx )
    {
    // 启动DMA发送
    sRes = HAL_UART_Transmit_DMA( huart, 
                                  (uint8_t*)pvBuff, 
                                  uNumBytes );
//    sRes = HAL_UART_Transmit(           huart,
//                            (uint8_t *)pvBuff,
//                                    uNumBytes,
//                                          200 );
//    if( 0 != UARTx_Is_HalfDuplex( pUartCtrl ) )  // 半双工状态?
//      {
//      UART_Listen( pUartCtrl );
//      }
    }
  else
    {
    // 中断发送
    sRes = HAL_UART_Transmit_IT( huart, 
                                 (uint8_t*)pvBuff, 
                                 uNumBytes );
    }

  return sRes;
}
//-----------------------------------------------------------------------------
// 检查Uart端口状态
// 当接收超时，检查端口状态，并复位
void UART_CheckPort(TUARTCtrlBlock *pUartCtrl)
{
  
  // 1. 检查发送使能线是否有效
  if(  0 != UARTx_Is_HalfDuplex( pUartCtrl ) && 
       0 != UARTx_Is_TxState( pUartCtrl ) )
    {
    // 重新开始接收
    UART_Listen( pUartCtrl );
    }
  else
    {
    if( &GetUARTChl( UARTCHL_MCU0 ) == pUartCtrl )
      {
      // 重新配置端口
      MX_UART1_Init();
      }

#if !(defined(UIB1_RST_ACTIVE) || defined(UIB2_RST_ACTIVE))
    // 重新开始接收
    UART_Listen( pUartCtrl );
#endif
    }
}
//-----------------------------------------------------------------------------
// 切换全部UART通道为接收状态
void CloseAllUartTxEn(void)
{

#if NUM_UART_MCU > 1  
  for( uint32_t uIdx = 0; uIdx < NUM_UART_MCU; uIdx++ )
    {
    TUARTCtrlBlock *pUartCtrl = &GetUARTChl(uIdx);
#else  
    {
    TUARTCtrlBlock *pUartCtrl = &GetUARTChl( UARTCHL_MCU0 );
#endif  
    
    // 等待发送结束
    if( USB_CPLSENT == (pUartCtrl->usState & USB_CPLSENT) )
      {
      UART_HandleTypeDef *huart = &(pUartCtrl->Uart);

      // 串口发送完？
      if( __HAL_UART_GET_FLAG( huart, UART_FLAG_TC ) != RESET )
        {
        // 重新开始接收
        UART_Listen( pUartCtrl );

        // 清标志
        pUartCtrl->usState &= ~USB_CPLSENT;
          
        // TOGGLE_DBUG; // debug only
        }
      }
    }
}
//-----------------------------------------------------------------------------
