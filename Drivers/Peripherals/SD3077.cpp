//-----------------------------------------------------------------------------
/*
 File        : SD3077.h
 Version     : V1.01
 By          : 银网科技

 Description :SD3077的驱动
        
 Date       : 2018.3.12

 v1.2: 2023.12.16 Wey
  修改Init方法
  修改SD3077_ReadState方法
  修改SD3077_ReadSN方法，，返回字符串
  增F32K输出
*/
//-----------------------------------------------------------------------------
#include "SD3077.h"

#include "Dev_Cfg.h"

#include "gpio.h"
#include "iwdg.h"

#include "CRC1632.h"

#include <cmsis_os.h>
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------
// SD3077指令
#define   SD3077_WRCMD                 0x64
#define   SD3077_RDCMD                 0x65
//-----------------------------------------------------------------------------
// RTC存器地址
#define   SD3077_RTC                   0x00
#define   SD3077_SEC                   0x00
#define   SD3077_MIN                   0x01
#define   SD3077_HOUR                  0x02
#define   SD3077_DAYOFWEEK             0x03
#define   SD3077_DAY                   0x04
#define   SD3077_MONTH                 0x05
#define   SD3077_YEAR                  0x06

// RTC控制寄存器地址
#define   SD3077_CTR1                  0x0F
#define      RTC_RTCF                  0x01  // 全部电源曾失效，有写操作即清零
#define      RTC_PMF                   0x02  // 当前是电池模式
#define      RTC_WRTC2                 0x04  // 
#define      RTC_BLF                   0x08  // 电池电压欠压
#define      RTC_INTDF                 0x10  // 
#define      RTC_INTAF                 0x20  // 
#define      RTC_OSF                   0x40  // 晶振曾停振
#define      RTC_WRTC3                 0x80  // 
#define   SD3077_CTR2                  0x10  // 
#define      RTC_INTFE                 0x01  // 频率中断允许
#define      RTC_INTAE                 0x02  // 报警中断
#define      RTC_INTDE                 0x04  // 倒计时中断
#define      RTC_FOBAT                 0x08  // 
#define      RTC_INTS0                 0x10  // 00=电量 01=报警 10=频率 11=倒计时
#define      RTC_INTS1                 0x20  // 
#define      RTC_IM                    0x40  // 
#define      RTC_WRTC1                 0x80  // 
#define   SD3077_CTR3                  0x11  // 
#define      RTC_FS0                   0x01  // 
#define      RTC_FS1                   0x02  // 
#define      RTC_FS2                   0x04  // 
#define      RTC_FS3                   0x08  // 
#define      RTC_TDS0                  0x10  // 
#define      RTC_TDS1                  0x20  // 
#define      RTC_F32K                  0x40  // 
#define      RTC_ARST                  0x80  // 自动复位CTR1使能，
#define   SD3077_TTF                   0x12  // 
#define      RTC_F0                    0x01  // 
#define      RTC_F1                    0x02  // 
#define      RTC_F2                    0x04  // 
#define      RTC_F3                    0x08  // 
#define      RTC_F4                    0x10  // 
#define      RTC_F5                    0x20  // 
#define      RTC_F6                    0x40  // 
#define      RTC_1PPM3PPM              0x80  // 
#define   SD3077_TEMPERATURE           0x16  // 温度
#define   SD3077_AGTC                  0x17  // IIC总线控制
#define      RTC_BATIIC                0x80 
#define   SD3077_CHARGE                0x18  // 充电选择
  #define    RTC_CHARGE_EN             0x80  // 允许充电
  #define    RTC_CHARGE_1              0x02  // 充电电阻
  #define    RTC_CHARGE_0              0x01
#define   SD3077_CTR4                  0x19
#define      RTC_INTBLE                0x01
#define      RTC_INTBHE                0x02
#define      RTC_INTTLE                0x04
#define      RTC_INTTHE                0x08
#define      RTC_CONT_BAT              0x10
#define      RTC_INS_E0                0x20
#define      RTC_INS_E1                0x40
#define      RTC_INS_E2                0x80
#define   SD3077_CTR5                  0x1A
#define      RTC_BLOW                  0x01  // 与RTC_BLF等价
#define      RTC_BHIGH                 0x02  // 电压电压高
#define      RTC_OSC_RDY               0x20
#define      RTC_SYS                   0x40
#define      RTC_BAT8_VAL              0x80
#define   SD3077_BATVOLTAGE            0x1B  // 电池电压

#define   SD3077_TEMPLOWALARMSET       0x1C
#define   SD3077_TEMPHIGHALARMSET      0x1D
#define   SD3077_TEMPLOWESTVALUE       0x1E
#define   SD3077_TEMPHIGHESTVALUE      0x1F
#define   SD3077_TEMPLOWESTTIME        0x20  // 历史最低温时刻，以下6个地址为分、时、星期、日、月、年
#define   SD3077_TEMPHIGHESTTIME       0x26

#define   SD3077_USER                  0x2C  // 0x2C ~ 0x71

#define   SD3077_IDENT                 0x72  // 0x72 ~ 0x79
  #define SD3077_IDENT_YEAR            0x72  // 出厂年00-99
  #define SD3077_IDENT_MONTH           0x73  // 出厂月01-12
  #define SD3077_IDENT_DAY             0x74  // 出厂日01-31
  #define SD3077_IDENT_MACNO           0x75  // 生产机台编号
  #define SD3077_IDENT_PON             0x76  // 生产工单序号
  #define SD3077_IDENT_PSN             0x78  // 工单内序号
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 管脚操作
#define   Set_SCL_High                 RTC_SCL_HIGH
#define   Set_SCL_Low                  RTC_SCL_LOW
#define   Set_SDA_High                 RTC_SDA_HIGH
#define   Set_SDA_Low                  RTC_SDA_LOW 
#define   Get_SDA_State                RTC_SDA_Input
//-----------------------------------------------------------------------------
#define  BCD2Hex(x)   (((x) >> 4) * 10 + ((x) & 0x0F))
//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
// 总线访问互斥
static osMutexId  mutexRTC = nullptr;
//=============================================================================
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
  // 对 SD3077 的所有操 作均必须由开始条件开始。 
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
  // 此时 SD3077 的所有操作均停止，总线进入待机状态。
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
* Function Name  : SD3077_Acknowledge Response
* Description    : Wiat I2C Bus Acknown
* Input          : None
* Output         : None
* Return         : 0: Ok  !=0: failed
*******************************************************************************/
static int iic_WaitAck(void)
{
  
  // 数据传输以 8 位序列进行。
  // SD3077 在第九个时钟周期时将 SDA 置位为低电平，
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
// 芯片寄寄存器操作
/*******************************************************************************
* Function Name  : SD3077_Write
* Description    : Write SD3077 Register
* Input          : ucAddr:     Start register address
*                  pucData:    Pointer of Data cache
*                  ucNumBytes: Bytes of data
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_Write( uint32_t uAddr, const void *pvBuf, uint32_t uNumBytes )
{
    
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( SD3077_WRCMD ) )
    {
    iic_Stop();
    return -2;
    }
  
  if( 0 != iic_SendByte( uAddr ) )   
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
* Function Name  : SD3077_Read
* Description    : Read some data from slave registers
* Input          : ucAddr:      Start register address
*                  pvBuf:       Pointer of Data cache
*                  uNumBytes:   Bytes of data
* Output         : None
* Return         : 0:成功  others=失败
*******************************************************************************/
static int SD3077_Read( uint32_t uAddr, void *pvBuf, uint32_t uNumBytes )
{
    
  if( 0 != iic_Start() )
    return -1;
  
  iic_Delay();
  
  if( 0 != iic_SendByte( SD3077_WRCMD ) )  // 0x64
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
    iic_Stop();
    return -5;
    }
  
  if( 0 != iic_SendByte( SD3077_RDCMD ) )  // 0x65
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
* Function Name  : SD3077_SetRegister
* Description    : SD3077 set register bits
* Input          : uAddr:   register address
*                  uSigns:  sign bits of register
*                  iSet:    0=clear bits   1=set bits
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_ReviseRegister( uint32_t uAddr, uint32_t uSigns, int iSet )
{
    
//  // -------- 
//  if( 0 != iic_Start() )
//    return -1;
//  
//  iic_Delay();
//  
//  if( 0 != iic_SendByte( SD3077_WRCMD ) )  // 0x64
//    {
//    iic_Stop();
//    return -2;
//    }
//  
//  if( 0 != iic_SendByte( uAddr ) )    
//    {
//    iic_Stop();
//    return -4;
//    }
//  
//  if( 0 != iic_Start() )
//    {
//    iic_Stop();
//    return -5;
//    }
//  
//  if( 0 != iic_SendByte( SD3077_RDCMD ) )  // 0x65
//    {
//    iic_Stop();
//    return -6;
//    }
//  
//  uint8_t ucData = iic_ReadByte();
//  iic_SendNAck();  // A_

//  iic_Stop();

  // -------- 读寄存器
   uint8_t ucData;
   if( 0 != SD3077_Read( uAddr, &ucData, 1 ) )
     return -1;

  if( 0 != iSet )
    // 修改标志位    
    ucData |= uSigns;
  else
    ucData &= ~uSigns;
  
  // -------- 回写
//  if( 0 != iic_Start() )
//    return -7;
//    
//  iic_Delay();
//  
//  if( 0 != iic_SendByte( SD3077_WRCMD ) )
//    {
//    iic_Stop();
//    return -8;
//    }
//  
//  if( 0 != iic_SendByte( uAddr ) )
//    {
//    iic_Stop();
//    return -9;
//    }

//  if( 0 !=  iic_SendByte( ucData ) )    
//    {
//    iic_Stop();
//    return -10;
//    }

//  iic_Stop();
    
  return SD3077_Write( uAddr, &ucData, 1 );
}

/*******************************************************************************
* Function Name  : SD3077_SetRegister
* Description    : SD3077 set register bits
* Input          : uAddr:   register address
*                  uSigns:  sign bits of register
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_SetRegister( uint32_t uAddr, uint32_t uSigns )
{
  
  return SD3077_ReviseRegister( uAddr, uSigns, 1 );
}
/*******************************************************************************
* Function Name  : SD3077_ClrRegister
* Description    : SD3077 clear register bits
* Input          : uAddr:   register address
*                  uSigns:  sign bits of register
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_ClrRegister( uint32_t uAddr, uint32_t uSigns )
{
    
  return SD3077_ReviseRegister( uAddr, uSigns, 0 );
}

/*******************************************************************************
* Function Name  : SD3077_WriteEnable
* Description    : SD3077 write enable
* Input          : None
*                  ucNumBytes: Bytes of data
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_WriteEnable()
{

//  if( 0 != iic_Start() )
//    return -1;
//  
//  if( 0 != iic_SendByte( SD3077_WRCMD ) )
//    {
//    iic_Stop();
//    return -2;
//    }
//  
//  if( 0 != iic_SendByte( SD3077_CTR2 ) ) // 0x10   
//    {
//    iic_Stop();
//    return -3;
//    }
//  
//  if( 0 != iic_SendByte( 0x80 ) )    // 置WRTC1=1 
//    {
//    iic_Stop();
//    return -4;
//    }

//  iic_Stop();

//  // 
//  if( 0 != iic_Start() )
//    {
//    return -5;
//    }
//  
//  if( 0 != ic_SendByte( SD3077_WRCMD ) )
//    {
//    iic_Stop();
//    return -6;
//    }
//  
//  if( 0 != iic_SendByte( SD3077_CTR1 ) )   // 0x0F 
//    {
//    iic_Stop();
//    return -7;
//    }
//  
//  if( 0 != iic_SendByte( 0x84 ) )    // 置WRTC2=1, WRTC3=1 
//    {
//    iic_Stop();
//    return -8;
//    }

//  iic_Stop();

  if( 0 != SD3077_SetRegister(SD3077_CTR2, RTC_WRTC1) )
    return -1;
  
  return SD3077_SetRegister(SD3077_CTR1, RTC_WRTC2 | RTC_WRTC3);
}  
/*******************************************************************************
* Function Name  : SD3077_WriteDisable
* Description    : SD3077 write disable
* Input          : None
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_WriteDisable()
{
  
//  if( 0 != iic_Start() )
//    return -1;
//  
//  if( 0 !=  iic_SendByte( SD3077_WRCMD ) )
//    {
//    iic_Stop();
//    return -2;
//    }
//  
//  if( 0 != iic_SendByte( SD3077_CTR1 ) )  // 0x0F  
//    {
//    iic_Stop();
//    return -3;
//    }
//  
//  if( 0 != iic_SendByte( 0x7B ) )    // 置WRTC2=0, WRTC3=0
//    {
//    iic_Stop();
//    return -4;
//    }

//  if( 0 != iic_SendByte( 0x7F ) )    // 置WRTC1=0 (地址自动增长)
//    {
//    iic_Stop();
//    return -5;
//    }

//  iic_Stop();

  if( 0 != SD3077_ClrRegister(SD3077_CTR1, RTC_WRTC2 | RTC_WRTC3) )
    return -1;
  
  return SD3077_ClrRegister(SD3077_CTR2, RTC_WRTC1);
}

/*******************************************************************************
* Function Name  : SD3077_F32K_Enable
* Description    : SD3077 F32K enable
* Input          : None
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
static int SD3077_F32K_Enable()
{
  
  uint8_t ucCTR3, ucCTR2;
  int iRes = SD3077_Read( SD3077_CTR3, &ucCTR3, 1 );
  if( 0 != iRes )
    return -1;
  
  iRes = SD3077_Read( SD3077_CTR2, &ucCTR2, 1 );
  if( 0 != iRes )
    return -2;

  #define CTR2_FREQ_OUT   (RTC_INTFE | RTC_INTS1)
  #define CTR3_F32K_OUT   (RTC_F32K  | RTC_FS0)  // RTC_FSx=32768在手册上未注
  if( CTR3_F32K_OUT == ucCTR3 && 
      CTR2_FREQ_OUT == ucCTR2 )
    return 0;

  // 允许写入
  if( 0 != SD3077_WriteEnable() )
    return -3;
  
  // 允许
  ucCTR3 = CTR3_F32K_OUT;  // CTR3中其它位全部清0
  iRes = SD3077_Write( SD3077_CTR3, &ucCTR3, 1 );
  if( 0 != iRes )
    return -4;
  
  // 配置INT管脚输出
  ucCTR2 = RTC_WRTC1 | CTR2_FREQ_OUT; // 一定要加WRTC1, 保持可写入
  iRes = SD3077_Write( SD3077_CTR2, &ucCTR2, 1 );
  if( 0 != iRes )
    return -5;
  
  // 禁止写入
  return SD3077_WriteDisable();
}
//=============================================================================
// 全局方法实现
//-----------------------------------------------------------------------------
/*******************************************************************************
* Function Name  : SD3077_Init
* Description    : Initializes I2C IO.
* Input          : None
* Output         : None
* Return         : 0：失败， 其它：芯片ID的hash
*******************************************************************************/
uint32_t SD3077_Init(void)
{
  
  Set_SDA_High;
  Set_SCL_High;
  
  iic_Start();
  iic_Stop();
  
  // 申请互斥
  if( nullptr == mutexRTC )
    {
    osMutexDef( RTC_MUTEX );
#if osCMSIS >= 0x20000U
    mutexRTC = osMutexNew( osMutex( RTC_MUTEX ) ); 
#else
    mutexRTC = osMutexCreate( osMutex( RTC_MUTEX ) ); 
#endif
    }
  
  // 读芯片ID
  uint8_t uIDBuf[8];
  int iIDRes = SD3077_ReadSN( uIDBuf ); // 01：1802-1B36-2403-0287 05:1802-1B35-2403-0283
  
  uint32_t uRes;
  if( 0 == iIDRes )
    {
    uRes = CRC32( uIDBuf, 8 );  // 01：9D09-1899 05:4EE8-2385
      
    SD3077_F32K_Enable();       // 允许32K输出
    }
  else
    uRes = 0;
    
  return uRes;
}
/*******************************************************************************
* Function Name  : SD3077_ReadState
* Description    : 读5个CTRx寄存器，综合其中状态，用SD3077_STATE_xx状态宏填写
* Input          : pState: 存放结果的数据区
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
// 读RTC状态
// 返回：  0 成功   !=0: 失败
int SD3077_ReadState(uint32_t *pState)
{
  
  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;
  
  uint8_t pucBuff[6];
  
  *pState = 0;
  
  // Read Time for SD3077
  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_CTR1, pucBuff, 3 );
    if( 0 == iRes )
      break;
    }

   if( 0 == iRes )
     {
     // Read Time for SD3077
     // 尝试5次
     iCnt = 5;
     while( --iCnt )
       {
       iRes = SD3077_Read( SD3077_CHARGE, pucBuff + 3, 3 );
       if( 0 == iRes )
         break;
       }
     }
  
  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iCnt )
    {
    // 
    uint32_t uState = pucBuff[0];  // CTR1
    if( 0 != (uState & RTC_RTCF) )
      *pState |= SD3077_STATE_RTCF;
    
    if( 0 != (uState & RTC_PMF) )
      *pState |= SD3077_STATE_PMF;
    
    if( 0 != (uState & RTC_OSF) )
      *pState |= SD3077_STATE_OSF;
    
    uState = pucBuff[2];  // CTR3
    if( 0 != (uState & RTC_F32K) )
      *pState |= SD3077_STATE_F32K;
    
    uState = pucBuff[3];  // SD3077_CHARGE
    if( 0 != (uState & RTC_CHARGE_EN) )
      *pState |= SD3077_STATE_CHARGE;
    
    uState = pucBuff[5];  // CTR5
    if( 0 != (uState & RTC_BLOW) )
      *pState |= SD3077_STATE_BLF;
    
    if( 0 != (uState & RTC_BHIGH) )
      *pState |= SD3077_STATE_BHF;
    }
  
  return iRes;
}
/*******************************************************************************
* Function Name  : SD3077_GetTime
* Description    : 读取时间
* Input          : pucTime：按时、分、秒顺序保存的时间
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int SD3077_GetTime( TDateTimeType *pDateTime )
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  uint8_t pucBuf[7];
  
  // Read Time for SD3077
  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_RTC, pucBuf, 3 );
    if( 0 == iRes )
      break;
    }

  if( 0 == iRes || 0 == iCnt )
    {
    // 格式转换
    pDateTime->Hours   = BCD2Hex( pucBuf[2] & 0x3F );
    pDateTime->Minutes = BCD2Hex( pucBuf[1] & 0x7F );
    pDateTime->Seconds = BCD2Hex( pucBuf[0] & 0x7F );
    
    if( 0x20 == (pucBuf[2] & 0xA0) )
      pDateTime->Hours += 12;
    
    // 无效的时间
    if( 23 < pDateTime->Hours || 59 < pDateTime->Minutes ||
        59 < pDateTime->Seconds )
      iRes = 3;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  return iRes;  
}
/*******************************************************************************
* Function Name  : SD3077_GetDateTime
* Description    : 读取日期时间
* Input          : pucTime：按年、月、日、时、分、秒顺序保存的时间, 年 - YEAR_BEGIN
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int SD3077_GetDateTime( TDateTimeType *pDateTime )
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  uint8_t pucBuf[10];
  
  // Read Time for SD3077
  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_RTC, pucBuf, 7 );
    if( 0 == iRes )
      break;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iRes )
    {
    // 格式转换
    pDateTime->Year    = BCD2Hex( pucBuf[6] ) + YEAR_BEGIN;
    pDateTime->Month   = BCD2Hex( pucBuf[5] & 0x1F );
    pDateTime->Day     = BCD2Hex( pucBuf[4] & 0x3F );
    pDateTime->WeekDay = BCD2Hex( pucBuf[3] & 0x07 );
    pDateTime->Hours   = BCD2Hex( pucBuf[2] & 0x3F );
    pDateTime->Minutes = BCD2Hex( pucBuf[1] & 0x7F );
    pDateTime->Seconds = BCD2Hex( pucBuf[0] & 0x7F );
    
    if( 0x20 == (pucBuf[2] & 0xA0) )
      pDateTime->Hours += 12;
    
    // 无效的时间
    if( 0 == pDateTime->Month || 12 < pDateTime->Month   ||
        0 == pDateTime->Day   || 59 < pDateTime->Day     ||
        23 < pDateTime->Hours || 59 < pDateTime->Minutes ||
        59 < pDateTime->Seconds )
      iRes = 3;
    }

  return iRes;  
}
/*******************************************************************************
* Function Name  : SD3077_SetDateTime
* Description    : 设置时间
* Input          : pDateTime：日期时间结构
* Output         : None
* Return         : 0 成功   !=0: 失败
*******************************************************************************/
int SD3077_SetDateTime( const TDateTimeType *pDateTime )
{  

  // 无效的时间
  if( 0 == pDateTime->Month || 12 < pDateTime->Month ||
      0 == pDateTime->Day   || 59 < pDateTime->Day ||
      23 < pDateTime->Hours || 59 < pDateTime->Minutes ||
      59 < pDateTime->Seconds )
    return 101;

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  int iRes;
  // 允许写入
  if( 0 == SD3077_WriteEnable() )
    {
    // 格式转换
    uint8_t pucBuf[7];
    pucBuf[6] = pDateTime->Year - YEAR_BEGIN;
    pucBuf[5] = pDateTime->Month; 
    pucBuf[4] = pDateTime->Day;  
    pucBuf[3] = pDateTime->WeekDay; 
    pucBuf[2] = pDateTime->Hours;
    pucBuf[1] = pDateTime->Minutes; 
    pucBuf[0] = pDateTime->Seconds; 
    
    // HEX -> BCD
    for( int iIdx = 0; iIdx < 7; iIdx++ )
      pucBuf[iIdx] = ((pucBuf[iIdx] / 10) << 4) | (pucBuf[iIdx] % 10);
    
    pucBuf[2] |= 0x80;  // 24小时制
    
    // 尝试5次
    int iCnt = 5;
    while( --iCnt )
      {
      iRes = SD3077_Write( SD3077_RTC, pucBuf, 8 );
      if( 0 != iRes )
        continue;
    
      // 清数字调整寄存器
      iRes = iic_Start();
      if( 0 != iRes )
        continue;
    
      if( 0 != iic_SendByte( SD3077_WRCMD ) ) // 0x64
        {
        iic_Stop();
        continue;
        }
        
      if( 0 != iic_SendByte( SD3077_TTF ) )   // 0x12
        {
        iic_Stop();
        continue;
        }
    
      if( 0 != iic_SendByte( 0 ) ) // 0
        {
        iic_Stop();
        continue;
        }
    
      break;
      }
    
    iic_Delay();
      
    // 禁止写入
    SD3077_WriteDisable();
    }
  else
    iRes = -2;

  // 释放总线
  osMutexRelease( mutexRTC );

  return iRes;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 读回当前温度
// 返回：  0 成功   !=0: 失败
int SD3077_GetTemperature(int* iTemp)
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 尝试5次
  int iRes, iCnt = 5;
  uint8_t uByte;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_TEMPERATURE, &uByte, 1 );
    if( 0 == iRes )
      break;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iRes )
    {
    *iTemp = uByte;
    if( 0 != (uByte & 0x80) )
      *iTemp -= 0x100;
    }

  return iRes;
}
//------------------------------------------------------------------------------
// 读回历史最低温度
// 返回：  0 成功   !=0: 失败
int SD3077_GetLowestTemp( int* iTemp, TDateTimeType *pDateTime )
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  uint8_t ucBuff[8];

  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_TEMPLOWESTVALUE, ucBuff, 8 );
    if( 0 == iRes )
      break;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iRes )
    {
    *iTemp = ucBuff[0];
    if( 0 != (ucBuff[0] & 0x80) )
      *iTemp -= 0x100;

    pDateTime->Year    = BCD2Hex( ucBuff[7] ) + YEAR_BEGIN;
    pDateTime->Month   = BCD2Hex( ucBuff[6] & 0x1F );
    pDateTime->Day     = BCD2Hex( ucBuff[5] & 0x3F );
    pDateTime->WeekDay = BCD2Hex( ucBuff[4] & 0x07 );
    pDateTime->Hours   = BCD2Hex( ucBuff[3] & 0x3F );
    pDateTime->Minutes = BCD2Hex( ucBuff[2] & 0x7F );
    pDateTime->Seconds = 0;
    }

  return iRes;
}
//------------------------------------------------------------------------------
// 读回历史最高温度
// 返回：  0 成功   !=0: 失败
int SD3077_GetHighestTemp( int* iTemp, TDateTimeType *pDateTime )
{

  uint8_t ucBuff[8];

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_TEMPHIGHESTVALUE, ucBuff, 1 );  //
    if( 0 == iRes )
      {
      iRes = SD3077_Read( SD3077_TEMPHIGHESTTIME, ucBuff + 2, 6 );  //
      if( 0 == iRes )
        break;
      }
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iRes )
    {
    *iTemp = ucBuff[0];
    if( 0 != (ucBuff[0] & 0x80) )
      *iTemp -= 0x100;

    pDateTime->Year    = BCD2Hex( ucBuff[7] ) + YEAR_BEGIN;
    pDateTime->Month   = BCD2Hex( ucBuff[6] & 0x1F );
    pDateTime->Day     = BCD2Hex( ucBuff[5] & 0x3F );
    pDateTime->WeekDay = BCD2Hex( ucBuff[4] & 0x07 );
    pDateTime->Hours   = BCD2Hex( ucBuff[3] & 0x3F );
    pDateTime->Minutes = BCD2Hex( ucBuff[2] & 0x7F );
    pDateTime->Seconds = 0;
    }

  return iRes;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 读电池电压
// 返回：  0 成功   !=0: 失败
int SD3077_ReadBVol(int* iVol)
{

  uint8_t ucBuff[2];

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_BATVOLTAGE - 1, ucBuff, 2 );
    if( 0 == iRes )
      break;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  if( 0 == iRes )
    {
    *iVol = ucBuff[1];
     if( 0 != (ucBuff[0] & 0x80) )
       *iVol += 256;
     }

  return iRes;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 读生产序列号
//  pID: 长度不少于8字节的空间
// 返回：  0 成功   !=0: 失败
int SD3077_ReadSN(uint8_t *pID)
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_IDENT, pID, 8 );
    if( 0 == iRes )
      {
      pID[0] = BCD2Hex( pID[0] );
      pID[1] = BCD2Hex( pID[1] );
      pID[2] = BCD2Hex( pID[2] );

      break;
      }
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  return iRes;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 写用户内存空间
// 返回：  0 成功   !=0: 失败
int SD3077_WriteMem( const void* pvData, uint32_t uBytesToWrite )
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 允许写入
  SD3077_WriteEnable();
  
  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Write( SD3077_USER, (uint8_t*)pvData, uBytesToWrite );
    if( 0 == iRes )
      break;
    }
    
  // 禁止写入
  SD3077_WriteDisable();

  // 释放总线
  osMutexRelease( mutexRTC );

  return iRes;
}
//-----------------------------------------------------------------------------
// 读用户内存空间
// 返回：  0 成功   !=0: 失败
int SD3077_ReadMem ( const void* pvData, uint32_t uBytesToRead )
{

  // 申请总线
  if( 0 != osMutexAcquire( mutexRTC, 200 ) )
    return -1;

  // 尝试5次
  int iRes, iCnt = 5;
  while( --iCnt )
    {
    iRes = SD3077_Read( SD3077_USER, (uint8_t*)pvData, uBytesToRead );
    if( 0 == iRes )
      break;
    }

  // 释放总线
  osMutexRelease( mutexRTC );

  return iRes;
}
//-----------------------------------------------------------------------------
