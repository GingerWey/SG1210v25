//-----------------------------------------------------------------------------
/*
 File        : MB85RSxx.c
 Version     : V1.10
 By          : 银网科技
 Description :MB85RSxx/FM25Lxx铁电IC驱动函数
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "MB85RSxx.h" 

#include "gpio.h"      
#include "SPIChls.h"

#include <dev_cfg.h>

#include <cmsis_os.h>
//=============================================================================
// 宏定义
//-----------------------------------------------------------------------------
#define MB85RS_HSPI         (SPICtrls[SPICHL_FeRAM].SPI)
#if VER_SG12B10 >= 120
   // v1.20 用SPI1, 速度84MHz
   // MB85RS SPI最高20MHz, 用8分频 = 10.5MHz
   #define MB85RS_SPI_BAUDRATEPRESCALER  SPI_BAUDRATEPRESCALER_8
#else
  #error 未知的主板版本
#endif

// 片选控制
#define MB85RS_nCS_Low()    { MB85RS_WAIT_MUTX;   FeRAM_nCS_LOW;   }
#define MB85RS_nCS_High()   { FeRAM_nCS_HIGH;    MB85RS_RELEASE_MUTX; }

// 写保护
#define MB85RS_nWP_Low()    FeRAM_nWP_LOW
#define MB85RS_nWP_High()   FeRAM_nWP_HIGH

// 挂起控制
#define MB85RS_nHold_Low()  
#define MB85RS_nHold_High() 

// Set SPI
// 不共享总线, 不改配置
#define MB85RS_SetSPI       MB85RS_SetSPI_Speed()
//-----------------------------------------------------------------------------
// EEROM的分页尺寸
// 用于铁电时，不分页，此处填0
#define MB85_SIZE_PAGE      0
#define MB85_SIZE           FeRAM_SIZE
//-----------------------------------------------------------------------------
// 操作命令
#define MB85_WREN           0x06    // Set Write Enable Latch 
#define MB85_WRDI           0x04    // Write Disable 
#define MB85_RDSR           0x05    // Read Status Register 
#define MB85_WRSR           0x01    // Write Status Register 
#define MB85_READ           0x03    // Read Memory Data 
#define MB85_WRITE          0x02    // Write Memory Data 
//=============================================================================
// 私用方法
//----------------------------------------------------------------------------
// 互斥访问
#define MB85RS_WAIT_MUTX     //osMutexWait( SPICtrls[SPICHL_FeRAM].Mutex, osWaitForever )     
#define MB85RS_RELEASE_MUTX  //osMutexRelease( SPICtrls[SPICHL_FeRAM].Mutex )
//=============================================================================
// 私用方法
//----------------------------------------------------------------------------
// 函数名:MB85RS_Send_Byte
// 功  能:读写SPI总线
// 注意事项:对于SPI来说，主机的读也需要先写，
// 这里使用直接操作寄存器的办法实现SPI硬件层读写,是为了加快速写速度
//-----------------------------------------------------------------------------
void MB85RS_SetSPI_Speed()
{
  
  // MB85RS最高速度20M
  if( MB85RS_SPI_BAUDRATEPRESCALER != MB85RS_HSPI.Init.BaudRatePrescaler )
    {
    MB85RS_WAIT_MUTX;

    // SPI configuration
    __HAL_SPI_DISABLE( &MB85RS_HSPI );      //必须先禁能,才能改变MODE

    MB85RS_HSPI.Init.BaudRatePrescaler = MB85RS_SPI_BAUDRATEPRESCALER;

    HAL_SPI_Init(&MB85RS_HSPI);

    __HAL_SPI_ENABLE(&MB85RS_HSPI); 

    MB85RS_RELEASE_MUTX;
    }
}
//-----------------------------------------------------------------------------
static void MB85RS_Send_Byte(uint8_t byte)
{
 
  __NOP();
  __NOP();
  __NOP();
  __NOP();
  
  HAL_SPI_Transmit( &MB85RS_HSPI, &byte, 1, 200 );
}
//----------------------------------------------------------------------------
// 函数名:MB85RS_Revc_Byte
// 功  能:读写SPI总线
// 注意事项:对于SPI来说，主机的读也需要先写，
// 使用此函数，读的时候建议参数设置为0xff，写的时候则写参数.
// 这里使用直接操作寄存器的办法实现SPI硬件层读写,是为了加快速写速度
//-----------------------------------------------------------------------------
static uint8_t MB85RS_Revc_Byte()
{
  
  uint8_t ucRes;
  
  __NOP();
  __NOP();
  __NOP();
  __NOP();

  if( HAL_SPI_Receive( &MB85RS_HSPI, &ucRes, 1, 200 ) != HAL_OK )
    ucRes = 0;
  
  return ucRes;
}
//-----------------------------------------------------------------------------
// 函数名:MB85RS_Send_Cmd
// 功能:写一个
// 注意事项:这是一个完整的单命令操作，不返回
//-----------------------------------------------------------------------------
void MB85RS_Send_Cmd(uint8_t cmd)
{

  MB85RS_nCS_Low();
  
  uint8_t ucHead[2];
  ucHead[0] = cmd;
  
  HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 1, 200 );
  
  //MB85RS_Send_Byte(cmd);
  MB85RS_nCS_High();
}
//-----------------------------------------------------------------------------
// 函数名:MB85RS_WREN
// 功能:写使能
//-----------------------------------------------------------------------------
void MB85RS_WREN(void)
{
  
  MB85RS_Send_Cmd(MB85_WREN);
}
//-----------------------------------------------------------------------------
// 函数名:MB85RS_WRDI
// 功能:禁止写操作
//-----------------------------------------------------------------------------
void MB85RS_WRDI(void)
{
  
  MB85RS_Send_Cmd(MB85_WRDI);
}
//-----------------------------------------------------------------------------
// 函数名:MB85RS_Read_StatusReg
// 功能:读状态寄存器
//-----------------------------------------------------------------------------
uint8_t MB85RS_Read_StatusReg()
{
  
  uint8_t ucRes;
 
  MB85RS_nCS_Low();
 
  MB85RS_Send_Byte(MB85_RDSR);

  ucRes = MB85RS_Revc_Byte();

  MB85RS_nCS_High();

  return( ucRes );
}
//-----------------------------------------------------------------------------
// 函数名:MB85RS_Write_StatusReg
// 功能:写状态寄存器
//-----------------------------------------------------------------------------
void MB85RS_Write_StatusReg(uint8_t ucData)
{

  MB85RS_nCS_Low();

  MB85RS_Send_Byte(MB85_WRSR);
  MB85RS_Send_Byte(ucData);

  MB85RS_nCS_High();
}
//-----------------------------------------------------------------------------
//  MB85RS_Write_SWPEn 写保护
//  注意事项: 写入比较繁琐，建议在每次操作前都取消掉写保护，操作完成后则重新允许写保护
//-----------------------------------------------------------------------------
void MB85RS_WriteDisable(void)
{
  
  uint8_t ucSReg = MB85RS_Read_StatusReg();
  ucSReg |= 0x8C;                          // 读出寄存器并加入保护位
  
  MB85RS_Send_Cmd(MB85_WREN);              // 允许写Status Register
  MB85RS_Write_StatusReg(ucSReg);         
  MB85RS_Send_Cmd(MB85_WRDI);
}

//-----------------------------------------------------------------------------
// 允许写
//-----------------------------------------------------------------------------
void MB85RS_WriteEnable(void)
{

  uint8_t ucSReg = MB85RS_Read_StatusReg();
  ucSReg &= (~0x8c);                        // 读出寄存器并消除保护位
  
  MB85RS_Send_Cmd(MB85_WREN);               // 允许写Status Register
  MB85RS_Write_StatusReg(ucSReg);           // 写状态寄存器
  MB85RS_Send_Cmd(MB85_WREN);               // 允许写Status Register
}
//-----------------------------------------------------------------------------
// 非阻塞发送完成
//-----------------------------------------------------------------------------
/*
void MB85RS_TxRxComplete( TSPICtrlBlock *pSPI )
{

  MB85RS_nCS_High();

  MB85RS_WriteDisable();

  MB85RSxx_EnabledWrite( 0 );
}
*/
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
//初始化IIC
//-----------------------------------------------------------------------------
void MB85RSxx_Init(void)
{

  MB85RS_SetSPI;
    
  MB85RS_nCS_High();
  MB85RS_nWP_Low();
  MB85RS_nHold_High();

  __HAL_SPI_ENABLE( &MB85RS_HSPI ); 
}
//-----------------------------------------------------------------------------
//指定地址开始读取指定长度数据，最长四字节，按MSB组合成long 
int MB85RSxx_ReadDWord( uint32_t  uReadAddr,     
                        uint32_t *puData,
                        uint8_t   ucLen )
{

  MB85RS_SetSPI;   // 配置SPI到适合的工作模式, 申请互斥
  
  if( uReadAddr > FeRAM_SIZE )
    return 0;
  else if( ucLen > 4 )
    ucLen = 4;
  
  MB85RS_nCS_Low();

  uint8_t  ucHead[3]; 
  ucHead[0] = MB85_READ;
  ucHead[1] = uReadAddr >> 8;
  ucHead[2] = uReadAddr;

  int iRes;
  if( HAL_OK != HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 3, 200 ) )
    iRes = 0;
  else if( HAL_OK != HAL_SPI_Receive( &MB85RS_HSPI, 
                                      (uint8_t *)puData, 
                                      ucLen, 
                                      1000 ) )
    iRes = 0;
  else
    iRes = ucLen;
  
  MB85RS_nCS_High();
    
  return iRes;
}                                  
//-----------------------------------------------------------------------------
//从指定地址开始读出指定长度的数据  

int MB85RSxx_Read( uint32_t  uReadAddr,
                       void *pvBuffer,   
                   uint32_t  uNumBytes )
{

  uint8_t *pucBuffer = (uint8_t*)pvBuffer;

  if( uReadAddr > FeRAM_SIZE )
    return 0;
  
  MB85RS_SetSPI;   // 配置SPI到适合的工作模式， 申请互斥
  
  MB85RS_nCS_Low();

  uint8_t  ucHead[3]; 
  ucHead[0] = MB85_READ;
  ucHead[1] = uReadAddr >> 8;
  ucHead[2] = uReadAddr;

  int      iRes;
  if( HAL_OK != HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 3, 200 ) )
    iRes = 0;
  else if( HAL_OK != HAL_SPI_Receive( &MB85RS_HSPI, 
                                      (uint8_t *)pucBuffer, 
                                      uNumBytes, 
                                      1000 ) )
    iRes = 0;
  else
    iRes = uNumBytes;

  MB85RS_nCS_High();
  
  return iRes;
}
                   
//-----------------------------------------------------------------------------
/*
// 从指定地址开始读出指定长度的数据         
// 用DMA方式
void MB85RSxx_DMARead(uint32_t  uReadAddr,
                          void *pvBuffer,   
                      uint32_t  uNumBytes)
{

  uint8_t *pucBuffer = (uint8_t*)pvBuffer;

  if( uReadAddr > FeRAM_SIZE )
    return ;
  
  MB85RS_SetSPI;   // 配置SPI到适合的工作模式， 申请互斥
  
  MB85RS_nCS_Low();

  uint8_t  ucHead[3]; 
  ucHead[0] = MB85_READ;
  ucHead[1] = uReadAddr >> 8;
  ucHead[2] = uReadAddr;

  if( HAL_OK == HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 3, 200 ) )
    {
    MB85_SPICTL.RxCplt = MB85RS_TxRxComplete;
      
    SPIChl_ReceiveNoB( MB85_SPICHL, pucBuffer, uNumBytes );
    }
}   
*/
//-----------------------------------------------------------------------------
//向指定地址开始写入指定长度的数据               
int MB85RSxx_Write(  uint32_t  uWriteAddr,
                   const void *pvBuffer, 
                     uint32_t  uNumBytes)
{

  uint8_t *pucBuffer = (uint8_t*)pvBuffer;

  if( uWriteAddr >= FeRAM_SIZE )
    return 0;
  
  MB85RS_SetSPI;   // 配置SPI到适合的工作模式， 申请互斥

  MB85RS_WriteEnable();
  
  MB85RS_nCS_Low();

  uint8_t  ucHead[3]; 
  ucHead[0] = MB85_WRITE;
  ucHead[1] = uWriteAddr >> 8;
  ucHead[2] = uWriteAddr;

  int iRes;
  if( HAL_OK != HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 3, 200 ) )
    iRes = 0;
  else if( HAL_OK != HAL_SPI_Transmit( &MB85RS_HSPI, 
                                       (uint8_t *)pucBuffer, 
                                       uNumBytes, 
                                       1000 ) )
    iRes = 0;
  else
    iRes = uNumBytes;

  MB85RS_nCS_High();
  
  MB85RS_WriteDisable();
  
  return iRes;
}                    
//-----------------------------------------------------------------------------
/*
// 向指定地址开始写入指定长度的数据               
// 用DMA方式
int MB85RSxx_DMAWrite( uint32_t    uWriteAddr,
                       const void *pvBuffer, 
                       uint32_t    uNumBytes )
{

  uint8_t *pucBuffer = (uint8_t*)pvBuffer;

  if( uWriteAddr >= FeRAM_SIZE )
    return 0;
  
  MB85RS_SetSPI;   // 配置SPI到适合的工作模式， 申请互斥

  MB85RS_WriteEnable();
  
  MB85RS_nCS_Low();

  uint8_t  ucHead[3]; 
  ucHead[0] = MB85_WRITE;
  ucHead[1] = uWriteAddr >> 8;
  ucHead[2] = uWriteAddr;

  int iRes;
  if( HAL_OK != HAL_SPI_Transmit( &MB85RS_HSPI, ucHead, 3, 200 ) )
    iRes = 0;
  else 
    {
    MB85_SPICTL.TxCplt = MB85RS_TxRxComplete;
      
    SPIChl_SendNoB( MB85_SPICHL, pucBuffer, uNumBytes );
    
    iRes = uNumBytes;
    }
  
  return iRes;
}
*/
//-----------------------------------------------------------------------------
// 允许/禁止写入
// iEnabled == TOKEN_WriteFeRAM_Enabled时，允许
void MB85RSxx_EnabledWrite( uint32_t iEnabled )
{
  
  if( TOKEN_WriteFeRAM_Enabled == iEnabled )
    MB85RS_nWP_High();
  else
    MB85RS_nWP_Low();
}
//-----------------------------------------------------------------------------
// 允许/禁止保持
// iEnabled == TOKEN_WriteFeRAM_Enabled时，允许
void MB85RSxx_EnabledHold( uint32_t iEnabled )
{

  if( TOKEN_WriteFeRAM_Enabled == iEnabled )
    MB85RS_nHold_Low();
  else
    MB85RS_nHold_High();
}
//-----------------------------------------------------------------------------
