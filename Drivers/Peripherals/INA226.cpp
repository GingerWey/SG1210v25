//-----------------------------------------------------------------------------
/*
 File        : INA226.cpp
 Version     : V1.01
 By          : 银网科技

 Description :INA226的驱动

 Date       : 2023.12.17
*/
//-----------------------------------------------------------------------------
#include "INA226.h"

#include "I2CBus.h"

//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// INA226存器地址
#define  INA226_CONFIG       0x00  // Configuration Register (R/W)初始值4127
#define  INA226_SHUNT_V      0x01  // Shunt Voltage (R)初始值0，分流电压测量值
#define  INA226_BUS_V        0x02  // Bus Voltage (R)初始值0，总线电压测量值
#define  INA226_POWER        0x03  // Power (R)初始值0，输出功率测量值
#define  INA226_CURRENT      0x04  // Current (R)初始值0，分流电阻电流计算值
#define  INA226_CALIB        0x05  // Calibration (R/W)，设置全量程和电流LSB
#define  INA226_MASK         0x06  // Mask/Enable (R/W)，报警设置和转换准备标志
#define  INA226_ALERTLMT     0x07  // Alert Limit (R/W)，报警阈值
#define  INA226_MANUF_ID     0xFE  // Manufacturer ID (R)，0x5449
#define  INA226_DIE_ID       0xFF  // Die ID (R),0x2260

#define  INA226_MODE_POWER_DOWN          (0<<0) // Power-Down
#define  INA226_MODE_TRIG_SHUNT_VOLTAGE  (1<<0) // Shunt Voltage, Triggered
#define  INA226_MODE_TRIG_BUS_VOLTAGE    (2<<0) // Bus Voltage, Triggered
#define  INA226_MODE_TRIG_SHUNT_AND_BUS  (3<<0) // Shunt and Bus, Triggered
#define  INA226_MODE_POWER_DOWN2         (4<<0) // Power-Down
#define  INA226_MODE_CONT_SHUNT_VOLTAGE  (5<<0) // Shunt Voltage, Continuous
#define  INA226_MODE_CONT_BUS_VOLTAGE    (6<<0) // Bus Voltage, Continuous
#define  INA226_MODE_CONT_SHUNT_AND_BUS  (7<<0) // Shunt and Bus, Continuous

// Shunt Voltage Conversion Time
#define  INA226_VSH_140uS                (0<<3)
#define  INA226_VSH_204uS                (1<<3)
#define  INA226_VSH_332uS                (2<<3)
#define  INA226_VSH_588uS                (3<<3)
#define  INA226_VSH_1100uS               (4<<3)
#define  INA226_VSH_2116uS               (5<<3)
#define  INA226_VSH_4156uS               (6<<3)
#define  INA226_VSH_8244uS               (7<<3)

// Bus Voltage Conversion Time (VBUS CT Bit Settings[6-8])
#define  INA226_VBUS_140uS               (0<<6)
#define  INA226_VBUS_204uS               (1<<6)
#define  INA226_VBUS_332uS               (2<<6)
#define  INA226_VBUS_588uS               (3<<6)
#define  INA226_VBUS_1100uS              (4<<6)
#define  INA226_VBUS_2116uS              (5<<6)
#define  INA226_VBUS_4156uS              (6<<6)
#define  INA226_VBUS_8244uS              (7<<6)

// Averaging Mode (AVG Bit Settings[9-11])
#define  INA226_AVG_1                    (0<<9)
#define  INA226_AVG_4                    (1<<9)
#define  INA226_AVG_16                   (2<<9)
#define  INA226_AVG_64                   (3<<9)
#define  INA226_AVG_128                  (4<<9)
#define  INA226_AVG_256                  (5<<9)
#define  INA226_AVG_512                  (6<<9)
#define  INA226_AVG_1024                 (7<<9)

// Reset Bit (RST bit [15])
#define  INA226_RESET_ACTIVE             (1<<15)
#define  INA226_RESET_INACTIVE           (0<<15)

// Mask/Enable Register
#define  INA226_MER_SOL                  (1<<15) // Shunt Voltage Over-Voltage
#define  INA226_MER_SUL                  (1<<14) // Shunt Voltage Under-Voltage
#define  INA226_MER_BOL                  (1<<13) // Bus Voltagee Over-Voltage
#define  INA226_MER_BUL                  (1<<12) // Bus Voltage Under-Voltage
#define  INA226_MER_POL                  (1<<11) // Power Over-Limit
#define  INA226_MER_CNVR                 (1<<10) // Conversion Ready
#define  INA226_MER_AFF                  (1<<4)  // Alert Function Flag
#define  INA226_MER_CVRF                 (1<<3)  // Conversion Ready Flag
#define  INA226_MER_OVF                  (1<<2)  // Math Overflow Flag
#define  INA226_MER_APOL                 (1<<1)  // Alert Polarity Bit
#define  INA226_MER_LEN                  (1<<0)  // Alert Latch Enable

#define  INA226_MANUF_ID_DEFAULT         0x5449
#define  INA226_DIE_ID_DEFAULT           0x2260
//-----------------------------------------------------------------------------
#ifdef DRV_I2CBus_H
// INA226从地址
// A0=GND A1=GND
#define  INA226_ADDR(n)        (0x80 | ((n) << 1))
//-----------------------------------------------------------------------------
#else // undefined DRV_I2CBus_H
#include "gpio.h"
#include "iwdg.h"
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// INA226读写命令
// A0=GND A1=GND
#define  INA226_WRCMD          0x80
#define  INA226_RDCMD          0x81

#define  INA226_CMD(cmd, n)  ((cmd) | ((n) << 1))
//-----------------------------------------------------------------------------
#define  I2CBus_Acquire()         0
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
* Function Name  : INA226_Write
* Description    : Write INA226 Register
* Input          : uAddr:      Start register address
*                  pucData:    Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int INA226_Write( uint32_t    uIC,    uint32_t uAddr, 
                         const void *pvData, uint32_t uNumBytes )
{
    
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( INA226_CMD(INA226_WRCMD, uIC) ) )
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uAddr ) )
     {
    iic_Stop();
    return -3;
    }

  const uint8_t *pucData = (const uint8_t*)pvData;
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
* Function Name  : INA226_Read
* Description    : Read INA226 Register
* Input          : uAddr:      Start register address
*                  pvBuf:      Pointer of Data cache
*                  uNumBytes:  Bytes of data
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int INA226_Read( uint32_t uIC,   uint32_t uAddr, 
                        void    *pvBuf, uint32_t uNumBytes )
{
    
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( INA226_CMD(INA226_WRCMD, uIC) ) )
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uAddr ) )  
    {
    iic_Stop();
    return -4;
    }
  
  if( 0 != iic_Start() )
    {
    return -5;
    }
  
  if( 0 != iic_SendByte( INA226_CMD(INA226_RDCMD, uIC) ) )
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
#endif
/*******************************************************************************
* Function Name  : INA226_Write
* Description    : Write 2 Bytes to INA226 Register
* Input          : uAddr:      Start register address
*                  uwData:     Data to write
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int INA226_Write( uint32_t uIC, uint32_t uAddr, uint16_t uwData )
{
  
  uint8_t pucBuf[2];
  
  pucBuf[0] = uwData >> 8;
  pucBuf[1] = uwData;

  int iRes, iCnt = 5;
  while( --iCnt )
    {
#ifndef DRV_I2CBus_H
    iRes = INA226_Write( uIC, uAddr, pucBuf, 2 );
#else
    iRes = I2CBus_WriteReg( INA226_ADDR(uIC), uAddr, pucBuf, 2 );
#endif
    if( 0 == iRes )
      break;
    }

  return iRes;
}
/*******************************************************************************
* Function Name  : INA226_Read
* Description    : Read  2 Bytes from INA226 Register
* Input          : uAddr:      Start register address
*                  puwData:    Pointer of Data cache
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int INA226_Read( uint32_t uIC, uint32_t uAddr, uint16_t *puwData )
{
  
  uint8_t pucBuf[2];

  int iRes, iCnt = 5;
  while( --iCnt )
    {
#ifndef DRV_I2CBus_H
    iRes = INA226_Read( uIC, uAddr, pucBuf, 2 );
#else
    iRes = I2CBus_ReadReg( INA226_ADDR(uIC), uAddr, pucBuf, 2 );
#endif
    }

  if( 0 == iRes )
    {
    *puwData = pucBuf[0] * 0x100 + pucBuf[1];
    }

  return iRes;
}
/*******************************************************************************
* Function Name  : INA226_Config
* Description    : Write config to INA226 Registers
* Input          : uAddr:      Start register address
*                  puwData:    Pointer of Data cache
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int INA226_Config(uint32_t uIC)
{
  
  // 申请总线占用
  I2CBus_Acquire();
  
  // 配置Config寄存器
  // 0x4527 = 0100_010_100_100_111
  // AVGx    010 = 16次平均
  // VBUSCTx 100 = 1.1ms
  // VSHCTx  100 = 1.1ms
  // MODEx   111 = Shunt and Bus, Continuous 连续测量分流电压和总线电压
    
  // 设置转换时间8.244ms,求平均值次数16，设置模式为分流和总线连续模式
  // 总数据转换时间 = 8.244*16 = 131.9ms
  // 0x45FF = 0100_010_111_111_111
  // AVGx    010 = 16次平均
  // VBUSCTx 111 = 8.244mS
  // VSHCTx  111 = 8.244mS
  // MODEx   111 = Shunt and Bus, Continuous 连续测量分流电压和总线电压
  int iRes = INA226_Write( uIC, INA226_CONFIG, 0x45FF );

  // 校准寄存器
  if( 0 == uIC )
    {
    // 充电回路
    // 分流电阻最大电压 = 32768 * 0.0000025V = 0.08192V
    // 设置分流电压转电流转换参数:电阻0.01R，分辨率0.2mA
    // 公式1
    //   Current_LSB = 预期最大电流 / 2^15
    //   Current_LSB = 5 / 32768 = 0.000152A = 0.152mA, 选0.2mA
    //   Imax = 0.0002 * 32768 = 6.5536A
    // 公式2
    //   CAL = 0.00512/(Current_LSB*R)
    //   CAL = 0.00512/(0.0002*0.01)=2560 = 0x0A00
    iRes |= INA226_Write( uIC, INA226_CALIB, 0xA00 );
    }
  else // Ver2.1 电池放电回路测量
    {
    // 放电回路
    // 分流电阻最大电压 = 32768 * 0.0000025V = 0.08192V
    // 设置分流电压转电流转换参数:电阻0.01R，分辨率1mA
    // 公式1
    //   Current_LSB = 预期最大电流 / 2^15
    //   Current_LSB = 32 / 32768 = 0.000977A = 0.9mA, 选1mA
    //   Imax = 0.001 * 32768 = 32.768A
    // 公式2
    //   CAL = 0.00512/(Current_LSB*R)
    //   CAL = 0.00512/(0.001*0.01)=512 = 0x0200
    iRes |= INA226_Write( uIC, INA226_CALIB, 0x200 );
    }

  // 释放总线占用
  I2CBus_Release();
  
  return iRes;
}
//=============================================================================
// 全局方法实现
//-----------------------------------------------------------------------------
/*******************************************************************************
* Function Name  : INA226_Init
* Description    : Initializes I2C IO.
* Input          : uIC: 芯片序号
* Output         : None
* Return         : 0：失败， 其它：芯片ID的hash
*******************************************************************************/
uint32_t INA226_Init(uint32_t uIC)
{
  
#ifndef DRV_I2CBus_H
  Set_SDA_High;
  Set_SCL_High;

  iic_Start();
  iic_Stop();
#else
  I2CBus_Init();
#endif
  
  uint32_t uIdent;
  int iIDRes = INA226_ReadIdent( uIC, &uIdent );

  if( 0 == iIDRes && 0x54492260 == uIdent )
    {
    if( 0 != INA226_Config( uIC ) )
      uIdent = 0;
    }
  else
    uIdent = 0;
    
  return uIdent;
}
/*******************************************************************************
* Function Name  : INA226_ReadConfig
* Description    : 读回Config寄存器
* Input          : uIC    :  芯片序号
*                  pResult: 存放结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int INA226_ReadConfig(uint32_t uIC, uint32_t *pResult)
{
  
  // 申请总线占用
  if( 0 != I2CBus_Acquire() )
    return -101;

  // Read Config for INA226
  int iRes = INA226_Read( uIC, INA226_CONFIG, (uint16_t*)pResult );

  // 释放总线占用
  I2CBus_Release();

  return iRes;
}
/*******************************************************************************
* Function Name  : INA226_ReadMeasure
* Description    : 读回测量结果
* Input          : uIC    :  芯片序号
*                  pResult: 存放结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int INA226_ReadMeasure(uint32_t uIC, TINA226Measure *pResult)
{
  
#ifndef DRV_I2CBus_H
  int iRes = INA226_Read( uIC, INA226_SHUNT_V, &(pResult->uwVoltShunt) );
  if( 0 != iRes )
    return iRes;

  iRes = INA226_Read( uIC, INA226_BUS_V, &(pResult->uwVoltBus) );
  if( 0 != iRes )
    return iRes;

  iRes = INA226_Read( uIC, INA226_POWER, &(pResult->uwPower) );
  if( 0 != iRes )
    return iRes;

  iRes = INA226_Read( uIC, INA226_CURRENT, &(pResult->uwCurrent) );
#else
  // 申请总线占用
  if( 0 != I2CBus_Acquire() )
    return -101;

  int iRes = 0, iOp;
  iOp = INA226_Read( uIC, INA226_SHUNT_V, &(pResult->uwVoltShunt) );
  if( 0 != iOp )
    iRes = iOp;

  iOp = INA226_Read( uIC, INA226_BUS_V, &(pResult->uwVoltBus) );
  if( 0 != iOp )
    iRes = iOp;

  iOp = INA226_Read( uIC, INA226_POWER, &(pResult->uwPower) );
  if( 0 != iOp )
    iRes = iOp;

  iOp = INA226_Read( uIC, INA226_CURRENT, &(pResult->uwCurrent) );
  if( 0 != iOp )
    iRes = iOp;

  // 释放总线占用
  I2CBus_Release();
#endif
  
  return iRes;
}
/*******************************************************************************
* Function Name  : INA226_ReadIdent
* Description    : 读回测量结果
* Input          : uIC    :  芯片序号
*                  pResult: 存放结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
// 读生产序列号
//  pID: 长度不少于8字节的空间
// 返回：  0 成功   !=0: 失败
int INA226_ReadIdent(uint32_t uIC, uint32_t *pResult)
{
  
  // 申请总线占用
  if( 0 != I2CBus_Acquire() )
    return -101;
  
  uint16_t uwMId, uwId;
  int iRes = 0, iOp;
  iOp = INA226_Read( uIC, INA226_MANUF_ID, &uwMId );
  if( 0 != iOp )
    iRes = iOp;

  iOp = INA226_Read( uIC, INA226_DIE_ID, &uwId );
  if( 0 != iOp )
    iRes = iOp;

  // 释放总线占用
  I2CBus_Release();

  if( 0 == iRes )
    {
    *pResult = uwMId * 0x10000 + uwId;
    }

  return iRes;
}
//-----------------------------------------------------------------------------
