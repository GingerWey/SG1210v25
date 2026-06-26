//-----------------------------------------------------------------------------
/*
 File        : MCP401X.cpp
 Version     : V1.01
 By          : 银网科技

 Description :MCP401X的驱动
              MCP4017: Rheostat
              MCP4018: Potentiometer
              MCP4019: Rheostat
        
 Date       : 2023.12.18

 
 2024/10/5
 Ver 2.1  Wey
   适配V2.1硬件
*/
//-----------------------------------------------------------------------------
#include "MCP401x.h"

#include "I2CBus.h"

#include "BoardCtrl.h"

#ifdef DRV_I2CBus_H
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// MCP401X地址
#define   MCP401X_ADDR             0x5E

#else
#include "gpio.h"
#include "iwdg.h"
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
#define   MCP401X_WRCMD            0x5E
#define   MCP401X_RDCMD            0x5F
//-----------------------------------------------------------------------------
#define  I2CBus_Acquire()          0
#define  I2CBus_Release()
//-----------------------------------------------------------------------------
// 管脚操作
#define  Set_SCL_High             SI2C_SCL_HIGH
#define  Set_SCL_Low              SI2C_SCL_LOW
#define  Set_SDA_High             SI2C_SDA_HIGH
#define  Set_SDA_Low              SI2C_SDA_LOW 
#define  Get_SDA_State            SI2C_SDA_Input
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

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
* Return         : None
*******************************************************************************/
static void iic_SendByte(uint32_t uAddr)
{
  
  //写8个位
  for( int iIdx = 8; iIdx > 0; iIdx-- )
    {
    // CLK = 0
    Set_SCL_Low;
    iic_Delay();
      
    if( 0 != (uAddr & 0x80) )
      Set_SDA_High;
    else
      Set_SDA_Low;

    uAddr <<= 1;

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
static uint8_t iic_ReadByte(void)
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
//==============================================================================
// 芯片读写
/*******************************************************************************
* Function Name  : MCP401X_Write
* Description    : Write one byte to MCP401X Register
* Input          : uData:      wiper data
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int MCP401X_Write( uint32_t uData )
{
    
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( MCP401X_WRCMD ) )
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uData ) )    
    {
    iic_Stop();
    return -3;
    }

  iic_Stop();
    
  return 0;
}
/*******************************************************************************
* Function Name  : MCP401X_Read
* Description    : Read one byte from MCP401X Register
* Input          : pucData:  保存结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int MCP401X_Read( void *pvData )
{

  if( 0 != iic_Start() )
    return -1;

  iic_Delay();

  if( 0 != iic_SendByte( MCP401X_RDCMD ) )
    {
    iic_Stop();
    return -6;
    }

  uint8_t *pucData = (uint8_t*)pvData;
  *pucData = iic_ReadByte();
  iic_SendNAck();

  iic_Stop();

  return 0;
}
#endif
//=============================================================================
// 全局方法实现
//-----------------------------------------------------------------------------
/*******************************************************************************
* Function Name  : MCP401X_Init
* Description    : Initializes I2C IO.
* Input          : uIC: 芯片序号
* Output         : None
* Return         : 0：失败， 其它：芯片ID的hash
*******************************************************************************/
uint32_t MCP401X_Init()
{
  
#ifndef DRV_I2CBus_H
  Set_SDA_High;
  Set_SCL_High;
  
  iic_Start();
  iic_Stop();
#else
  I2CBus_Init();
#endif
  
  uint32_t uWiper = 0;
  int iIDRes = MCP401X_ReadWiper( &uWiper );
  if( 0 != iIDRes )
    uWiper = 0;
    
  return uWiper;
}
/*******************************************************************************
* Function Name  : MCP401X_ReadConfig
* Description    : 读MCP401X的分接头位置
* Input          : pResult: 存放结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int MCP401X_ReadWiper(uint32_t *pResult)
{

  // 申请总线占用
  if( 0 != I2CBus_Acquire() )
    return -101;

  // Read Wiper for MCP401X
  // 偿试5次
  int iTryCnt = 5, iRes;
  while( iTryCnt-- )
    {
    *pResult = 0;
#ifndef DRV_I2CBus_H
    iRes = MCP401X_Read( pResult );
#else
    iRes = I2CBus_Read( MCP401X_ADDR, pResult, 1 );
#endif
    if( 0 == iRes )
      break;
    }

  // 释放总线占用
  I2CBus_Release();

  return iRes;
}
/*******************************************************************************
* Function Name  : MCP401X_SetWiper
* Description    : 设置MCP401X的分接头位置
* Input          : uWiper: 新的分节头位置0...127,  包含在宏TOKEN_BRDSET内
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int MCP401X_SetWiper( uint32_t uToken )
{

  // 申请总线占用
  if( 0 != I2CBus_Acquire() )
    return -101;

  int iRes;
  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    uint32_t uWiper = TOKEN_BRDSET_GET( uToken );
    if( uWiper > 127 )
      return -100;
   
    // 偿试5次
    int iTryCnt = 5;
    while( iTryCnt-- )
      {
#ifndef DRV_I2CBus_H
      iRes = MCP401X_Write( uWiper );
#else
      iRes = I2CBus_Send( MCP401X_ADDR, &uWiper, 1 );
#endif
      if( 0 != iRes )
        continue;
        
      uint32_t uResult = 0;
#ifndef DRV_I2CBus_H
      iRes = MCP401X_Read( &uResult );
#else
      iRes = I2CBus_Read( MCP401X_ADDR,  &uResult, 1 );
#endif
      if( 0 != iRes )
        continue;
        
      if( uResult == uWiper )
        break;
      else
        iRes = -101;
      }
    }
  else
    iRes = -500;

  // 释放总线占用
  I2CBus_Release();

  return iRes;
}
//-----------------------------------------------------------------------------
