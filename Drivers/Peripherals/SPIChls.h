//-----------------------------------------------------------------------------
/*
 File        : SPIChls.h
 Version     : V1.10
 By          : 银网科技
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef __SPICHLs_H
#define __SPICHLs_H

#include "stm32f4xx_hal.h"

#include "dev_cfg.h"
#include "cmsis_os.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
 extern "C" {
#endif
//===========================================================================
// 全局常量
//----------------------------------------------------------------------------

//===========================================================================
// 全局宏定义
//----------------------------------------------------------------------------
// SPI通道
// 硬件 v2.2
#if VER_SG12B10 >= 210
 #define  SPICHL_NvRAM   0      // 铁电和Flash
 #define  SPICHL_FeRAM   0      // 铁电  MB85RS/FM25 最高20MHz
 #define  SPICHL_FLASH   0      // Flash W25Q        最高133MHz
   
 #define  NUM_SPICHLs    1
 #define  SPI1_CHL       SPICHL_NvRAM
#elif VER_SG12B10 >= 110
 #define  SPICHL_W5500   0      // W5500             最高80MHz
 #define  SPICHL_NvRAM   0      // 铁电和Flash
 #define  SPICHL_FeRAM   0      // 铁电  MB85RS/FM25 最高20MHz
 #define  SPICHL_FLASH   0      // Flash SST25VF     最高80MHz
 
 #define  NUM_SPICHLs    1
 #define  SPI1_CHL       SPICHL_W5500   
#else
  #error 未知的主板版本
#endif

// SPI状态
#define  SSB_IDLE       0x0000    // 空闲
#define  SSB_SENDING    0x0001    // 正在发送中
#define  SSB_REVING     0x0002    // 正在接收
#define  SSB_PAUSED     0x8000    // 通讯中断
//===========================================================================
// 全局数据结构
//---------------------------------------------------------------------------
typedef struct tagSPICtrlBlock
{
  
  SPI_HandleTypeDef  SPI;
  
  void              (*RxCplt)( struct tagSPICtrlBlock *pSPICtl );
  void              (*TxCplt)( struct tagSPICtrlBlock *pSPICtl );
  
  osMutexId          Mutex;       // 线程同步
  osSemaphoreId      SimRxEnd;    // 接收完成信号
  osSemaphoreId      SimTxEnd;    // 发送完成信号
  
  uint16_t           usState;
  
  DMA_HandleTypeDef  rxDMA;
  DMA_HandleTypeDef  txDMA;
  
  uint16_t           uwUser;      // 当前用户(共用状态)
  
} TSPICtrlBlock;
//===========================================================================
// 全局数据
//----------------------------------------------------------------------------
// 各UART通道的控制结构
extern TSPICtrlBlock  SPICtrls[NUM_SPICHLs];
//===========================================================================
// 全局方法
//----------------------------------------------------------------------------
// 初始化各SPI信道
// 此方法必须在DevOpts装载后方可使用
void SPIChl_Init(void);
// 此方法必须在操作系统初始化后使用
void SPIChl_OSSim(void);     // 创建OS同步信号
//----------------------------------------------------------------------------
// SPI接收信息，阻塞方式
HAL_StatusTypeDef SPIChl_Receive(uint8_t uChl, uint8_t *pData, uint16_t Size);
// SPI接收信息，非阻塞方式
HAL_StatusTypeDef SPIChl_ReceiveNoB(uint8_t uChl, uint8_t *pData, uint16_t Size);

// SPI发送信息，阻塞方式
HAL_StatusTypeDef SPIChl_Send(uint8_t uChl,  uint8_t *pData, uint16_t Size);
// SPI发送信息，非阻塞方式
HAL_StatusTypeDef SPIChl_SendNoB(uint8_t uChl,  uint8_t *pData, uint16_t Size);
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // __UARTCHLS_H
