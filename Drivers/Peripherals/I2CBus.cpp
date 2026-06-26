//-----------------------------------------------------------------------------
/*
 File        : I2CBus.cpp
 Version     : V1.01
 By          : 银网科技

 Description : I2C总线驱动
        
 Date        : 2023.12.17
*/
//-----------------------------------------------------------------------------
#include "I2CBus.h"

#include "gpio.h"
#include "iwdg.h"

#include <cmsis_os.h>
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// 管脚操作
#define  Set_SCL_High            SI2C_SCL_HIGH
#define  Set_SCL_Low             SI2C_SCL_LOW
#define  Set_SDA_High            SI2C_SDA_HIGH
#define  Set_SDA_Low             SI2C_SDA_LOW 
#define  Get_SDA_State           SI2C_SDA_Input
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
// 总线访问互斥
static osMutexId  mutexI2CBus = nullptr;
//==============================================================================
// IIC总线时序操作
/*******************************************************************************
* Function Name  : iic_Delay
* Description    : Short delay
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_Delay(void)
{
  // 30 = 2.9us/SCL Pulse = 344k < 400k
  for( int i = 30; i > 0; i-- )
    {
    __NOP();
    __NOP();
    }
}
//-----------------------------------------------------------------------------
/*******************************************************************************
* Function Name  : iic_Start
* Description    : Starting I2C Bus
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static int iic_Start(void)
{
  
  // 当 SCL 处于高电平时，SDA 由高电平变成低电平时构成一个开始条件，
  // 对 Slave 的所有操 作均必须由开始条件开始。 
  Set_SDA_High;
  iic_Delay();
  Set_SCL_High;
  iic_Delay();
  
  int iCnt = 100;
  while( --iCnt > 0 )
    {
    if( 0 != Get_SDA_State )
      break;
    }
  if( 0 == iCnt )
    return -1;
  
  Set_SDA_Low;
  iic_Delay();
  Set_SCL_Low;
  iic_Delay();
  
  return 0;
}
/*******************************************************************************
* Function Name  : iic_Stop
* Description    : Stop I2C Bus
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_Stop(void)
{
  
  // 当 SCL 处于高电平时，SDA 由低电平变成高电平时构成一个停止条件，
  // 此时 Slave 的所有操作均停止，总线进入待机状态。
  Set_SDA_Low ;
  iic_Delay();
  Set_SCL_Low ;
  iic_Delay();
  
  Set_SCL_High;
  iic_Delay();
  Set_SDA_High ;
  iic_Delay();
}

/*******************************************************************************
* Function Name  : iic_BusStop
* Description    : Stop I2CBus
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_BusStop(void)
{
  
  //时钟开始
  iic_Start();

  iic_Delay();
  
  //停止总线
  iic_Stop();
}
/*******************************************************************************
* Function Name  : iic_ClkPluse
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_ClkPluse(void)
{
  iic_Delay();
  Set_SCL_High;
  iic_Delay();
  Set_SCL_Low;
}
/*******************************************************************************
* Function Name  : iic_SendAck
* Description    : Send I2C Bus Acknown 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_SendAck(void)
{

  Set_SCL_Low;
  iic_Delay();
  Set_SDA_Low;
  iic_Delay();
  Set_SCL_High;
  iic_Delay();
  Set_SCL_Low;
  iic_Delay();
}
/*******************************************************************************
* Function Name  : iic_SendNack
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void iic_SendNAck(void)
{

  Set_SCL_Low;
  iic_Delay();
  Set_SDA_High;
  iic_Delay();
  
  Set_SCL_High;
  iic_Delay();
  Set_SCL_Low;
  iic_Delay();
}
/*******************************************************************************
* Function Name  : iic_Acknowledge Response
* Description    : Wiat I2C Bus Acknown
* Input          : None
* Output         : None
* Return         : 0: Ok  !=0: failed
*******************************************************************************/
static int iic_WaitAck(void)
{
  
  // 数据传输以 8 位序列进行。
  // Slave 在第九个时钟周期时将 SDA 置位为低电平，
  // 即送出一 个确认信号（Acknowledge bit,以下简称“ACK”），表明数据已经被其收到。
  Set_SCL_Low;
  iic_Delay();
  Set_SDA_High;
  iic_Delay();
  Set_SCL_High;
  iic_Delay();

  int iRes;
  if( 0 == Get_SDA_State )
    iRes = 0;
  else
    iRes = 1;
  
  Set_SCL_Low;
  iic_Delay();
  
  IWDG_FeedDog();   // 2020.10.29
  
  return iRes;
}
/*******************************************************************************
* Function Name  : iic_SendByte
* Description    : Send one byte data to I2C Device
* Input          : uData :  Data to be send
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
static int iic_SendByte(uint32_t uData)
{
  
  //写8个位
  for( int iIdx = 8; iIdx > 0; iIdx-- )
    {
    // CLK = 0
    Set_SCL_Low;
    iic_Delay();
      
    if( 0 != (uData & 0x80) )
      Set_SDA_High;
    else
      Set_SDA_Low;

    uData <<= 1;

    iic_Delay();
    Set_SCL_High;
    iic_Delay();
    }

  Set_SCL_Low;
  
  // 等待对方响应
  int iRes = iic_WaitAck();
  if( 0 != iRes )
    iic_Stop();
    
  return iRes;
}
/*******************************************************************************
* Function Name  : iic_ReadByte
* Description    : Receive one byte data from I2C  Device
* Input          : None
* Output         : None
* Return         : 读回的结果
*******************************************************************************/
static uint32_t iic_ReadByte(void)
{ 
    
  uint32_t uRes = 0;

  Set_SDA_High;
  // 读8个位
  for (int iIdx = 7; iIdx >= 0; --iIdx)
    {
    uRes <<= 1;
      
    Set_SCL_Low;
    iic_Delay();
    
    Set_SCL_High;
    iic_Delay();

    // 读取数据 读5次判断状态
    int iHigh = 0;
    for( int iTry = 0; iTry < 5; iTry++ )
      {
      if( 0 != Get_SDA_State )
        iHigh++;
      __NOP();
      }
    
    if( iHigh > 2 ) 
      uRes |= 1;
    }

  Set_SCL_Low;

  IWDG_FeedDog();   // 2020.10.29

  return uRes; 
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
/*******************************************************************************
* Function Name  : I2CBus_WriteReg
* Description    : Write some data into slave registers
* Input          : uSlave:     Slave address
*                  uReg:       Start Register Number
*                  pvBuf:      Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
// 写寄存器数据
// 返回：  0 成功   !=0: 失败
int I2CBus_WriteReg( uint32_t    uSlave, uint32_t uReg, 
                     const void *pvBuf,  uint32_t uNumBytes)
{
  
  
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( uSlave ) )
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uReg ) )   
    {
    iic_Stop();
    return -3;
    }

  const uint8_t *pucData = (const uint8_t*)pvBuf;
  while( uNumBytes-- )
    {
    if( 0 != iic_SendByte( *pucData++ ) )
      {
      iic_Stop();
      return -4;
      }
    }

  iic_Stop();
    
  return 0;
}
/*******************************************************************************
* Function Name  : I2CBus_ReadReg
* Description    : Read some data from slave registers
* Input          : uSlave:     Slave address
*                  uReg:       Register Number
*                  pvBuf:      Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
// 读寄存器数据
// 返回：  读回的数据
int I2CBus_ReadReg( uint32_t    uSlave, uint32_t uReg, 
                    const void *pvBuf,  uint32_t uNumBytes)
{
  
 if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( uSlave ) )    // WR Command
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uReg ) )   
    {
    iic_Stop();
    return -4;
    }
  
  if( 0 != iic_Start() )
    {
    return -5;
    }
  
  if( 0 != iic_SendByte( uSlave | 0x01 ) ) // RD Command
    {
    iic_Stop();
    return -6;
    }
  
  uint8_t *pucData = (uint8_t*)pvBuf;
  for( uint32_t uIdx = uNumBytes; uIdx > 1; uIdx-- )
    {
    *pucData = iic_ReadByte();
    iic_SendAck();
    
    pucData++;
    }
    
  *pucData = iic_ReadByte();
  iic_SendNAck();

  iic_Stop();
    
  return 0;
}
/*******************************************************************************
* Function Name  : I2CBus_Read
* Description    : read some data from slave registers
* Input          : uSlave:     Slave address
*                  pvBuf:      Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
// 直接写数据
// 返回：  0 成功   !=0: 失败
int I2CBus_Send(uint32_t uSlave, const void *pvBuf, uint32_t uNumBytes)
{
  
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( uSlave ) )      // 涉及的芯片均是R/nW
    {
    iic_Stop();
    return -2;
    }
  
  const uint8_t *pucData = (const uint8_t*)pvBuf;
  while( uNumBytes-- )
    {
    if( 0 != iic_SendByte( *pucData++ ) )    
      {
      iic_Stop();
      return -4;
      }
    }

  iic_Stop();
    
  return 0;
}
/*******************************************************************************
* Function Name  : I2CBus_Read
* Description    : read some data from slave registers
* Input          : uSlave:     Slave address
*                  pvBuf:      Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
// 直接读数据
// 返回：  读回的数据
int I2CBus_Read(uint32_t uSlave, const void *pvBuf, uint32_t uNumBytes)
{
  
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( uSlave | 0x01 ) )  // 涉及的芯片均是R/nW
    {
    iic_Stop();
    return -6;
    }
  
  uint8_t *pucData = (uint8_t*)pvBuf;
  for( uint32_t uIdx = uNumBytes; uIdx > 1; uIdx-- )
    {
    *pucData = iic_ReadByte();
    iic_SendAck();
    
    pucData++;
    }
    
  *pucData = iic_ReadByte();
  iic_SendNAck();

  iic_Stop();
    
  return 0;
}
/*******************************************************************************
* Function Name  : I2CBus_Init
* Description    : 初始化 I2C bus
* Input          : None
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
// 初始化
// 0：成功， 其它：失败
int I2CBus_Init()
{
  
  Set_SCL_High;
  Set_SDA_High;
  
  iic_Start();
  iic_Stop();
  
  if( nullptr == mutexI2CBus )
    {
    osMutexDef( I2CBUS_MUTEX );
#if osCMSIS >= 0x20000U
    mutexI2CBus = osMutexNew( osMutex( I2CBUS_MUTEX ) ); 
#else
    mutexI2CBus = osMutexCreate( osMutex( I2CBUS_MUTEX ) ); 
#endif
    }
    
  return (nullptr == mutexI2CBus)? 0 : -1;
}
/*******************************************************************************
* Function Name  : I2CBus_Acquire
* Description    : 申请占用总线
* Input          : None
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
int I2CBus_Acquire()
{
  
  if( nullptr == mutexI2CBus )
    return -101;
    
  osStatus_t sRes = osMutexAcquire( mutexI2CBus, 200 ); 
  
  return sRes;
}
/*******************************************************************************
* Function Name  : I2CBus_Release
* Description    : 释放占用总线
* Input          : None
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
int I2CBus_Release()
{

  if( nullptr == mutexI2CBus )
    return -101;

  osStatus_t sRes = osMutexRelease( mutexI2CBus); 
  
  return sRes;
}
