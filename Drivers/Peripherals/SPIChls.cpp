//-----------------------------------------------------------------------------
/*
 File        : SPIChls.c
 Version     : V1.10
 By          : 银网科技
 Description :SPIx信道要用到的结构和方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "SPIChls.h"

#include "spi.h"

#include <cmsis_os.h>
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
TSPICtrlBlock  SPICtrls[NUM_SPICHLs];
//===========================================================================
// 全局方法
//----------------------------------------------------------------------------
// 初始化各UART信道
void SPIChl_Init(void)
{
  
  MX_SPI1_Init();
}
//----------------------------------------------------------------------------
// 此方法必须在操作系统初始化后使用
void SPIChl_OSSim(void)     // 创建OS同步信号
{  

  // 初始化SPI信道的UartCtrls
  for( int iIdx = 0; iIdx < NUM_SPICHLs; iIdx++ )
    {
    SPICtrls[iIdx].usState = SSB_IDLE;
      
    osMutexDef( SPIx_MUTX );
#if osCMSIS >= 0x20000U
    SPICtrls[iIdx].Mutex = osMutexNew( osMutex( SPIx_MUTX ) ); 
#else
    SPICtrls[iIdx].Mutex = osMutexCreate( osMutex( SPIx_MUTX ) ); 
#endif

    osSemaphoreDef(Sim_SPI_Rx);
#if osCMSIS >= 0x20000U
    SPICtrls[iIdx].SimRxEnd = osSemaphoreNew( 4, 1, osSemaphore(Sim_SPI_Rx) );
#else
    SPICtrls[iIdx].SimRxEnd = osSemaphoreCreate( osSemaphore(Sim_SPI_Rx) , 1 );
#endif
      
    osSemaphoreDef(Sim_SPI_Tx);
#if osCMSIS >= 0x20000U
    SPICtrls[iIdx].SimTxEnd = osSemaphoreNew( 4, 1, osSemaphore(Sim_SPI_Rx) );
#else
    SPICtrls[iIdx].SimTxEnd = osSemaphoreCreate( osSemaphore(Sim_SPI_Tx) , 1 );
#endif
    }
}
//----------------------------------------------------------------------------
// SPI接收信息，阻塞方式
HAL_StatusTypeDef SPIChl_Receive(uint8_t uChl, uint8_t *pData, uint16_t Size)
{
  
  HAL_StatusTypeDef sRes;

  SPI_HandleTypeDef *pSPI = &SPICtrls[uChl].SPI;
  
  SPICtrls[uChl].usState |= SSB_REVING;

  sRes = HAL_SPI_Receive( pSPI, pData, Size, 200 );
  
  return sRes;
}
//----------------------------------------------------------------------------
// SPI接收信息，非阻塞方式
HAL_StatusTypeDef SPIChl_ReceiveNoB(uint8_t uChl, uint8_t *pData, uint16_t Size)
{
  
  HAL_StatusTypeDef sRes;

  SPI_HandleTypeDef *pSPI = &SPICtrls[uChl].SPI;
  
  SPICtrls[uChl].usState |= SSB_REVING;

  if( pSPI->hdmarx )
    sRes = HAL_SPI_Receive_DMA( pSPI, pData, Size );
  else
    sRes = HAL_SPI_Receive_IT( pSPI, pData, Size );
  
  return sRes;
}  
//----------------------------------------------------------------------------
// SPI发送信息，阻塞方式
HAL_StatusTypeDef SPIChl_Send(uint8_t uChl,  uint8_t *pData, uint16_t Size)
{
  
  HAL_StatusTypeDef sRes;
  
  SPI_HandleTypeDef *pSPI = &SPICtrls[uChl].SPI;

  SPICtrls[uChl].usState |= SSB_SENDING;
  
  sRes = HAL_SPI_Transmit( pSPI, pData, Size, 200 );
  
  return sRes;
}
//----------------------------------------------------------------------------
// SPI发送信息，非阻塞方式
HAL_StatusTypeDef SPIChl_SendNoB(uint8_t uChl,  uint8_t *pData, uint16_t Size)
{
  
  HAL_StatusTypeDef sRes;
  
  SPI_HandleTypeDef *pSPI = &SPICtrls[uChl].SPI;

  SPICtrls[uChl].usState |= SSB_SENDING;
  
  if( pSPI->hdmatx )
    sRes = HAL_SPI_Transmit_DMA( pSPI, pData, Size );
  else
    sRes = HAL_SPI_Transmit_IT( pSPI, pData, Size );
  
  return sRes;
}  
//===========================================================================
// 回调函数
//----------------------------------------------------------------------------
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

  TSPICtrlBlock *pSPICtl = 0;

  for( int iIdx = 0; iIdx < NUM_SPICHLs; iIdx++ )
    {
    if( hspi->Instance == SPICtrls[iIdx].SPI.Instance )
      {
      pSPICtl = &SPICtrls[iIdx];
      break;
      }
    }
    
  if( pSPICtl )
    {
    if( pSPICtl->usState & SSB_REVING )
      {
      if( pSPICtl->SimRxEnd )
        osSemaphoreRelease( pSPICtl->SimRxEnd );
      
      pSPICtl->usState &= ~SSB_REVING;
      
      if( pSPICtl->RxCplt )
        {
        pSPICtl->RxCplt( pSPICtl );
        pSPICtl->RxCplt = 0;
        }
      }
      
    if( pSPICtl->usState & SSB_SENDING )
      {
      if( pSPICtl->SimTxEnd )
        osSemaphoreRelease( pSPICtl->SimTxEnd );
      
      pSPICtl->usState &= ~SSB_SENDING;
      
      if( pSPICtl->TxCplt )
        {
        pSPICtl->TxCplt( pSPICtl );
        pSPICtl->TxCplt = 0;
        }
      }      
    }
}
//----------------------------------------------------------------------------
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{

  //HAL_SPI_TxRxCpltCallback( hspi );

  TSPICtrlBlock *pSPICtl = 0;

#if NUM_SPICHLs > 3
  for( int iIdx = 0; iIdx < NUM_SPICHLs; iIdx++ )
    {
    if( hspi->Instance == SPICtrls[iIdx].SPI.Instance )
      {
      pSPICtl = &SPICtrls[iIdx];
      break;
      }
    }
#elif NUM_SPICHLs > 0
    if( hspi->Instance == SPICtrls[0].SPI.Instance )
      pSPICtl = &SPICtrls[0];
  #if NUM_SPICHLs > 1
    else if( hspi->Instance == SPICtrls[1].SPI.Instance )
      pSPICtl = &SPICtrls[1];
   #if NUM_SPICHLs == 3
    else if( hspi->Instance == SPICtrls[2].SPI.Instance )
      pSPICtl = &SPICtrls[2];
   #endif
  #endif
#else
  return ;
#endif
  
  if( 0 == pSPICtl || pSPICtl->RxCplt )
    pSPICtl->RxCplt( pSPICtl );
}
//----------------------------------------------------------------------------
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{

  //HAL_SPI_TxRxCpltCallback( hspi );
  TSPICtrlBlock *pSPICtl = 0;

#if NUM_SPICHLs > 3
  for( int iIdx = 0; iIdx < NUM_SPICHLs; iIdx++ )
    {
    if( hspi->Instance == SPICtrls[iIdx].SPI.Instance )
      {
      pSPICtl = &SPICtrls[iIdx];
      break;
      }
    }
#elif NUM_SPICHLs > 0
    if( hspi->Instance == SPICtrls[0].SPI.Instance )
      pSPICtl = &SPICtrls[0];
  #if NUM_SPICHLs > 1
    else if( hspi->Instance == SPICtrls[1].SPI.Instance )
      pSPICtl = &SPICtrls[1];
   #if NUM_SPICHLs == 3
    else if( hspi->Instance == SPICtrls[2].SPI.Instance )
      pSPICtl = &SPICtrls[2];
   #endif
  #endif
#else
  return ;
#endif
    
  if( 0 == pSPICtl || pSPICtl->TxCplt )
    pSPICtl->TxCplt( pSPICtl );  
}
//----------------------------------------------------------------------------
