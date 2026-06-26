//-----------------------------------------------------------------------------
/*
 File        : SPIFlash.cpp
 Version     : V1.10
 By          : 银网科技

 Description : SPI Flash驱动函数
        
 Date        : 2023.12.05

 2018.12.11  银网科技
  自动检测芯片型号, 并选择读写算法

 2023.12.15 银网科技
   Wey.
   增加ZD25Q系列芯片支持

 2024.10.02 银网科技
   Wey.
   增加W25Q64JV系列芯片支持
*/
//-----------------------------------------------------------------------------
#include "SPIFlash.h" 

#include "SPIChls.h"           
#include "cmsis_os.h"
#include "gpio.h" 

#include "DevRegs.h"

#include <string.h>
//=============================================================================
// 宏定义
//-----------------------------------------------------------------------------
#define SFLASH_SPICHL          SPICHL_FLASH
#define SFLASH_SPICTRL         (SPICtrls[SFLASH_SPICHL])
#define SFLASH_HSPI            (SFLASH_SPICTRL.SPI)
#if VER_SG12B10 >= 120
   // v1.20 用SPI1, 速度84MHz
   // W25Qxx最高133MHz, 用j取快的2分频 = 42MHz
   #define SFLASH_SPI_BAUDRATEPRESCALER  SPI_BAUDRATEPRESCALER_2
#else
  #error 未知的主板版本
#endif

// 片选控制
#define Set_nCS_Low()          FLASH_nCS_LOW
#define Set_nCS_High()         FLASH_nCS_HIGH
// 写保护                      
#define Set_nWP_Low()          FLASH_nWP_LOW
#define Set_nWP_High()         FLASH_nWP_HIGH
// 挂起控制
#define SPIFLASH_nHold_Low()  
#define SPIFLASH_nHold_High() 

#define SPI_Read(p, n)         HAL_SPI_Receive ( &SFLASH_HSPI, (uint8_t*)(p), n, 200 )
#define SPI_Write(p, n)        HAL_SPI_Transmit( &SFLASH_HSPI, (uint8_t*)(p), n, 200 )

// SST25VF16在AAI模式下可以用SO检测Busy
#define SPIFLASH_MISO_PIN      NvRAM_MISO_Pin
#define SPIFLASH_MISO_PORT     NvRAM_MISO_Port
#define SPIFLASH_MISO_STAT     NvRAM_MISO_Input
//-----------------------------------------------------------------------------
// 操作命令
#define FLASH_READ      0x03  // Read Memory Data @ 33MHz
#define FLASH_HiREAD    0x0B  // Read Memory Data @ 80MHz-064 otherwise=50MHz
#define FLASH_ERASE4    0x20  // Erase 4 KByte of memory array
#define FLASH_ERASE32   0x52  // Erase 32 KByte of memory array
#define FLASH_ERASE64   0xD8  // Erase 64 KByte of memory array
#define FLASH_ERACHIP   0x60  // Erase Full Memory Array
#define FLASH_WRITE     0x02  // Page Program 

#define FLASH_RDSR      0x05  // Read Status Register
                              // The Read-Status-Register is continuous with ongoing clock cycles 
                              // until terminated by a low to high transition on CE# 
#define FLASH_WRSR      0x01  // Write Status Register 
#define FLASH_WREN      0x06  // Write Enable
#define FLASH_WRDI      0x04  // Write Disable 

#define FLASH_RDID      0x90  // Read-ID
                              // Manufacturer’s ID is read with A0=0, and Device ID is read with A0=1.
                              // All other address bits are 00H
                              // The Manufacturer’s ID and device ID output stream is continuous 
                              // until terminated by a low-to-high transition on CE#.
#define FLASH_RDJID     0x9F  // JEDEC ID read

//// --------  SST25VF016/032指令 --------------
//#define SST25_EAAIP     0xAD  // Auto Address Increment Programming
//                              // To continue programming to the next sequential address location,
//                              // enter the 8-bit command, ADH, followed by 2 bytes of data to be programmed. 
//                              // Data Byte 0 will be programmed into the initial address [A23-A1] with A0=0,
//                              // Data Byte 1 will be programmed into the initial address [A23-A1] with A0=1,
//#define SST25_EBSY      0X70  // Enable  SO to output RY/BY# status during AAI programming 
//#define SST25_DBSY      0X80  // Disable SO to output RY/BY# status during AAI programming 

// --------  SST25VF064指令 --------------
#define SST25_EWSR      0x50  // Enable-Write-Status-Register
#define SST25_EHLD      0xAA  // Enable HOLD# pin functionality of the RST#/HOLD# pin
#define SST25_RSID      0x88  // Read Security ID
#define SST25_PSID      0xA5  // Program User Security ID area
#define SST25_LSID      0x85  // Lockout Security ID Programming

// SST25 状态寄存器
#define S25SR_BUSY      0x01  // 1 = Internal Write operation is in progress 
#define S25SR_WEL       0x02  // 1 = Device is memory Write enabled
#define S25SR_BPx       0x3C  // 

#define S25SR_AAI       0x40  // 1 = AAI programming mode     VF016/032
#define S25SR_SEC       0x40  // 1 = Security ID space locked VF064

#define S25SR_BPL       0x80  // 1 = BPx are read-only bits

// --------  W25Qxx指令 --------------
#define W25Q_EWSR       0x50  // Enable-Write-Status-Register
#define W25Q_RDSR1      0x05  // Read Status1 Register
#define W25Q_RDSR2      0x35  // Read Status2 Register
#define W25Q_RDSR3      0x15  // Read Status3 Register
                        
#define W25Q_WRSR1      0x01  // Write Status1 Register
#define W25Q_WRSR2      0x31  // Write Status2 Register
#define W25Q_WRSR3      0x11  // Write Status3 Register

#define W25Q_RSTEN      0x66  // Reset Enable 
#define W25Q_RESET      0x99  // Reset

#define W25Q_RSREG      0x2B  // Read  Security Register
#define W25Q_WRREG      0x2F  // Write Security Register
                        
#define W25Q_WRSFDP     0x5A  // Read Serial Flash Discovery Parameter

// ZD25 状态寄存器
#define W25Q_BUSY   0x0001  // 1 = Internal Write operation is in progress 
#define W25Q_WEL    0x0002  // 1 = Device is memory Write enabled
#define W25Q_BPx    0x001C  // Block Protect
#define W25Q_TB     0x0020  // Top/Bottom Write Protect 

#define W25Q_SEC    0x0040  // 1 = Sector protect

#define W25Q_SRP0   0x0080  // Status Register Protect 0
#define W25Q_SRP1   0x0100  // Status Register Protect 1

#define W25Q_QE     0x0200  // Quad Enable 

#define W25Q_SRLB   0x1C00  // Security Register Lock Bits

#define W25Q_CMP    0x4000  // Complement Protect 

#define W25Q_SUS    0x8000  // Suspend Status 

//---------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 芯片参数
// SSTVF016
#define SST016_TYPE            SST25VF016
#define SST016_CAPACITY        SST25VF016
#define SST016_SIZE_SECTOR     4096UL
#define SST016_SIZE_PAGE       0
#define SST016_SECTORS        (SST016_CAPACITY / SST016_SIZE_SECTOR)

// SSTVF032 
#define SST032_TYPE            SST25VF032
#define SST032_CAPACITY        SST25VF032
#define SST032_SIZE_SECTOR     4096UL
#define SST032_SIZE_PAGE       256UL
#define SST032_SECTORS        (SST032_CAPACITY / SST064_SIZE_SECTOR)

// SSTVF064 
#define SST064_TYPE            SST25VF064
#define SST064_CAPACITY        SST25VF064
#define SST064_SIZE_SECTOR     4096UL
#define SST064_SIZE_PAGE       256UL
#define SST064_SECTORS        (SST064_CAPACITY / SST064_SIZE_SECTOR)

// W25Qxx
#define CAPACITY(x)            (W25Q32JV << ((x) - W25Q32JV_JID))
#define W25QxxJV_SIZE_SECTOR   4096UL
#define W25QxxJV_SIZE_PAGE     256UL
#define W25QxxJV_SECTORS(x)    ((x) / W25QxxJV_SIZE_SECTOR)
//=============================================================================
// 私用数据
//-----------------------------------------------------------------------------
static uint32_t    m_FlashJID;

#define SPIFLASH_WAIT_MUTX     //osMutexWait( SFLASH_SPICTRL.Mutex, osWaitForever )     
#define SPIFLASH_RELEASE_MUTX  //osMutexRelease( SFLASH_SPICTRL.Mutex )
//=============================================================================
// 私用方法
//-----------------------------------------------------------------------------
// 函数名：_SPI_Config
// 功  能：重新配置SPI
//-----------------------------------------------------------------------------
// 改变SPI的配置，适应
// SSTVF064: 80MHz
// SSTVF016: 50MHz
static void _SPI_Config(void)
{
  
  // 时钟频率正确时，不处理
  if( SFLASH_SPI_BAUDRATEPRESCALER != SFLASH_HSPI.Init.BaudRatePrescaler )
    {
    SPIFLASH_WAIT_MUTX;

    // SPI configuration
    __HAL_SPI_DISABLE( &SFLASH_HSPI );      //必须先禁能,才能改变MODE

    SFLASH_HSPI.Init.BaudRatePrescaler = SFLASH_SPI_BAUDRATEPRESCALER;

    HAL_SPI_Init(&SFLASH_HSPI);

    __HAL_SPI_ENABLE(&SFLASH_HSPI);
      
    SPIFLASH_RELEASE_MUTX;
    }
}
//-----------------------------------------------------------------------------
// 函数名：_Send_Cmd
// 功  能：写一个命令
// 注意事项:这是一个完整的单命令操作，不返回
//-----------------------------------------------------------------------------
void _Send_Cmd(uint8_t cmd)
{
    
  Set_nCS_Low();
  
  SPI_Write( &cmd, 1);
  
  Set_nCS_High();
}
//-----------------------------------------------------------------------------
// 函数名：SPIFLASH_WREN
// 功  能：写使能
//-----------------------------------------------------------------------------
void SPIFLASH_WREN(void)
{
  _Send_Cmd(FLASH_WREN);
}
//-----------------------------------------------------------------------------
// 函数名:SPIFLASH_WRDI
// 功能:禁止写操作
//-----------------------------------------------------------------------------
void SPIFLASH_WRDI(void)
{
  _Send_Cmd(FLASH_WRDI);
}
//-----------------------------------------------------------------------------
// 函数名: SST25_SetAAIPin
// 功  能: 切换MISO管脚功能, 仅用于AAI写入模式
//-----------------------------------------------------------------------------
#ifdef SST25_EAAIP
void SST25_SetAAIPin(int iAAIen)
{
  
  GPIO_InitTypeDef GPIO_InitStruct;

  if( 0 != iAAIen )
    {
    // 配置MISO为输入检测
    GPIO_InitStruct.Pin  = SPIFLASH_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(SPIFLASH_MISO_PORT, &GPIO_InitStruct);
    }
  else
    {
     // 配置MISO为SPI功能
    GPIO_InitStruct.Pin   = SPIFLASH_MISO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(SPIFLASH_MISO_PORT, &GPIO_InitStruct);
   }
}
//-----------------------------------------------------------------------------
// 函数名: SPIFLASH_EBSY
// 功  能: 允许SO输入Busy状态， 仅用于AAI写入模式
//-----------------------------------------------------------------------------
void SPIFLASH_EBSY(void)
{

  _Send_Cmd(SST25_EBSY);
  
  SST25_SetAAIPin( 0xAA );
}
//-----------------------------------------------------------------------------
// 函数名: SPIFLASH_DBSY
// 功  能: 禁止SO输入Busy状态
//-----------------------------------------------------------------------------
void SPIFLASH_DBSY(void)
{

  _Send_Cmd(SST25_DBSY);

  SST25_SetAAIPin( 0 );
}
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 函数名：_Read_SST25_StatusReg
// 功  能：读SST25系列状态寄存器
//-----------------------------------------------------------------------------
static uint32_t _Read_SST25_StatusReg()
{

  Set_nCS_Low();
 
  uint8_t ucCmd = FLASH_RDSR;
  SPI_Write( &ucCmd, 1 );

  uint8_t ucRes;
  SPI_Read( &ucRes, 1);

  Set_nCS_High();

  return ucRes ;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 函数名：_Read_ZD25_StatusReg
// 功  能：读ZD25系列状态寄存器
//-----------------------------------------------------------------------------
static uint32_t _Read_W25Q_StatusReg()
{

  // get SR1
  Set_nCS_Low();
 
  uint8_t ucCmd = W25Q_RDSR1;
  SPI_Write( &ucCmd, 1 );

  uint32_t uRes, uData = 0;
  SPI_Read( &uData, 1);

  Set_nCS_High();
  
  uRes = uData;

  // get SR2
  Set_nCS_Low();
 
  ucCmd = W25Q_RDSR2;
  SPI_Write( &ucCmd, 1 );

  SPI_Read( &uData, 1);

  Set_nCS_High();

  uRes |= uData << 8;

  // get SR3
  Set_nCS_Low();
 
  ucCmd = W25Q_RDSR3;
  SPI_Write( &ucCmd, 1 );

  SPI_Read( &uData, 1);

  Set_nCS_High();

  uRes |= uData << 16;
  
  return uRes ;
}
//-----------------------------------------------------------------------------
// 函数名：_Write_SST25_StatusReg
// 功  能：读SST25系列状态寄存器
//-----------------------------------------------------------------------------
static void _Write_SST25_StatusReg(uint32_t uData)
{

  Set_nCS_Low();
  
  uint8_t ucCmd = SST25_EWSR;
  SPI_Write( &ucCmd, 1 );
  
  // CE# must be driven low before the EWSR instruction is entered and 
  // must be driven high before the EWSR instruction is executed.
  Set_nCS_High();
  
  // CE# High Time > 50ns
  int iDelay = 50;
  while( --iDelay )
    __NOP();
  
  Set_nCS_Low();

  uint8_t  ucHead[2]; 
 
  ucHead[0] = FLASH_WRSR;
  ucHead[1] = uData;
  
  SPI_Write( ucHead, 2 );

  Set_nCS_High();
}
//-----------------------------------------------------------------------------
// 函数名：_Write_ZD25_StatusReg
// 功  能：读ZD25系列状态寄存器
//-----------------------------------------------------------------------------
static void _Write_W25Q_StatusReg(uint32_t uData)
{

  
  uint32_t uOld = _Read_W25Q_StatusReg();
  
  if( uData == uOld )
    return ;

  Set_nCS_Low();
  
  uint8_t ucCmd = W25Q_EWSR;
  SPI_Write( &ucCmd, 1 );

  Set_nCS_High();

  // CE# High Time > 45%PC
  int iDelay = 10;
  while( --iDelay )
    __NOP();

  Set_nCS_Low();

  uint8_t  ucHead[2];
  ucHead[0] = W25Q_WRSR1;
  ucHead[1] = uData;
  
  SPI_Write( ucHead, 2 );

  // CE# High Time > 45%PC
  iDelay = 10;
  while( --iDelay )
    __NOP();

  Set_nCS_Low();

  ucHead[0] = W25Q_WRSR2;
  ucHead[1] = uData >> 8;
  
  SPI_Write( ucHead, 2 );

  Set_nCS_High();

  // CE# High Time > 45%PC
  iDelay = 10;
  while( --iDelay )
    __NOP();

  Set_nCS_Low();

  ucHead[0] = W25Q_WRSR3;
  ucHead[1] = uData >> 16;
  
  SPI_Write( ucHead, 2 );

  Set_nCS_High();
}
//-----------------------------------------------------------------------------
// 函数名：_Read_StatusReg
// 功  能：读状态寄存器
//-----------------------------------------------------------------------------
static uint32_t _Read_StatusReg()
{

  uint32_t uResult;
  
  switch( m_FlashJID )
    {
    case SST016_JID:
    case SST032_JID:
    case SST064_JID:
      uResult = _Read_SST25_StatusReg();
      break;
    
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      uResult = _Read_W25Q_StatusReg();
      break;
    
    default:
      uResult = 0;
    }

  return uResult;  
}
//-----------------------------------------------------------------------------
// 函数名：_Write_StatusReg
// 功  能：写状态寄存器
//-----------------------------------------------------------------------------
static void _Write_StatusReg(uint32_t uData)
{

  switch( m_FlashJID )
    {
    case SST016_JID:
    case SST032_JID:
    case SST064_JID:
      _Write_SST25_StatusReg( uData );
      break;
    
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      _Write_W25Q_StatusReg( uData );
      break;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  SPIFLASH_Write_SWPEn 写保护
//  注意事项: 写入比较繁琐，建议在每次操作前都取消掉写保护，操作完成后则重新允许写保护
//-----------------------------------------------------------------------------
static uint32_t _SST25_WriteDisable(void)
{

  uint32_t uSReg;
  uSReg  = _Read_StatusReg();    // 读出寄存器并加入保护位
  uSReg &= ~S25SR_BPL;
  
  _Send_Cmd(FLASH_WREN);         // 允许写Status Register
  
  uSReg |= S25SR_BPx;
  _Write_StatusReg(uSReg);
  
  uSReg |= S25SR_BPL;
  _Write_StatusReg(uSReg);
  
  _Send_Cmd(FLASH_WRDI);
  
  uSReg = _Read_StatusReg();     // 读出SR寄存器
  return uSReg;
}
//-----------------------------------------------------------------------------
static uint32_t _W25Q_WriteDisable(void)
{


  uint32_t uSReg;
  uSReg  = _Read_StatusReg();         // 读出寄存器并加入保护位
  
  // SRP1 SRP0  nWP   Status-Register-Protection
  //   0    1    1    When nWP pin is high the Status register is unlocked and 
  //                  can be written to after a Write Enable instruction, WEL=1 
  //
  // CMP SEC TB TBx   Memory-Protection
  //  0   x   x 111       All
  uSReg |=  W25Q_SRP0 | W25Q_BPx;
  uSReg &= ~(W25Q_SRP1 | W25Q_CMP);

  _Send_Cmd(FLASH_WREN);              // 允许写Status Register
  _Write_StatusReg(uSReg);
  
  _Send_Cmd(FLASH_WRDI);
  
  uSReg = _Read_StatusReg();          // 读出SR寄存器
  return uSReg;
}
//-----------------------------------------------------------------------------
static uint32_t _SST25_WriteEnable(void)
{

  uint32_t uSReg;
  uSReg  = _Read_StatusReg();         // 读出寄存器并消除保护位

  uSReg &= ~(S25SR_BPx | S25SR_BPL);

  _Send_Cmd(FLASH_WREN);              // 允许写Status Register
  _Write_StatusReg(uSReg);            // 写状态寄存器
  
  uSReg |= S25SR_BPL;
  _Send_Cmd(FLASH_WREN);              // 允许写Status Register
  _Write_StatusReg(uSReg);            // 写状态寄存器
  
  _Send_Cmd(FLASH_WREN);              // 允许写Status Register

  uSReg = _Read_StatusReg();          // 读出SR寄存器
  return uSReg;
}
//-----------------------------------------------------------------------------
static uint32_t _W25Q_WriteEnable(void)
{

  uint32_t uSReg;
  uSReg  = _Read_StatusReg();         // 读出寄存器并加入保护位
  
  // SRP1 SRP0  nWP   Status-Register-Protection
  //   0    1    1    When nWP pin is high the Status register is unlocked and 
  //                  can be written to after a Write Enable instruction, WEL=1 
  //
  // CMP SEC TB TBx   Memory-Protection
  //  0   x   x  0        None
  uSReg &= ~(W25Q_SRP1 | W25Q_BPx | W25Q_CMP);
  uSReg |= W25Q_SRP0;

  _Send_Cmd(FLASH_WREN);              // 允许写Status Register
  _Write_StatusReg(uSReg);

  _Send_Cmd(FLASH_WRDI);
  
  uSReg = _Read_StatusReg();          // 读出SR寄存器
  return uSReg;
}
//-----------------------------------------------------------------------------
static uint32_t _WriteDisable(void)
{

  uint32_t uRes;
  
  switch( m_FlashJID )
    {
    case SST016_JID:
    case SST032_JID:
    case SST064_JID:
      uRes = _SST25_WriteDisable();
      break;
    
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      uRes = _W25Q_WriteDisable();
      break;
    
    default:
      uRes = 0;
    }

  return uRes;
}
//-----------------------------------------------------------------------------
// 允许写
//-----------------------------------------------------------------------------
static uint32_t _WriteEnable(void)
{

  uint32_t uRes;
  
  switch( m_FlashJID )
    {
    case SST016_JID:
    case SST032_JID:
    case SST064_JID:
      uRes = _SST25_WriteEnable();
      break;
    
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      uRes = _W25Q_WriteEnable();
      break;
    
    default:
      uRes = 0;
    }

  return uRes;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 检查SST25芯片“忙”状态
// 通过内部寄存器完成
static uint8_t _SST25_BusyChk( int iInAAI )
{
  
#ifdef SST25_EAAIP
  if( (SST016_JID == m_FlashJID || SST032_JID == m_FlashJID) && 0 != iInAAI )
    SST25_SetAAIPin( 0 );
#endif
  uint32_t uRes = _Read_StatusReg();

#ifdef SST25_EAAIP
  if( (SST016_JID == m_FlashJID || SST032_JID == m_FlashJID) && 0 != iInAAI )
    SST25_SetAAIPin( 0xAA );
#endif

  return (uRes & S25SR_BUSY);
}
//-----------------------------------------------------------------------------
// 检查ZD25Q芯片“忙”状态
// 通过内部寄存器完成
static uint8_t _W25Q_BusyChk()
{
  
  uint32_t uRes = _Read_StatusReg();

  return (uRes & W25Q_BUSY);
}
//-----------------------------------------------------------------------------
// 检查芯片“忙”状态
// 通过内部寄存器完成
static uint32_t _CheckBusy( int iInAAI )
{
  
  uint32_t uRes;

  switch( m_FlashJID )
    {
    case SST016_JID:
    case SST032_JID:
    case SST064_JID:
      uRes = _SST25_BusyChk( iInAAI );
      break;
    
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      uRes = _W25Q_BusyChk();
      break;
    
    default:
      uRes = 0;
    }

  return uRes;
}
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
//初始化IIC
//-----------------------------------------------------------------------------
void SPIFLASH_Init(void)
{
    
  Set_nCS_High();
  Set_nWP_Low();
  SPIFLASH_nHold_High();
  
//#if osCMSIS >= 0x20000U
//    FSST25Mutex = osMutexNew( nullptr ); 
//#else
//    FSST25Mutex = osMutexCreate( nullptr ); 
//#endif
}
//-----------------------------------------------------------------------------
// 从指定地址开始读出指定长度的数据
// 速度50MHz/80Mhz  
uint32_t SPIFLASH_Read( uint32_t  uwReadAddr,
                            void *pvBuffer,   
                        uint32_t  uNumBytes)
{

  uint32_t uRes;
  uint8_t  ucHead[5]; 

  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  switch( m_FlashJID )
    {
    case SST016_JID:
      {
      if( uwReadAddr + uNumBytes > SST016_CAPACITY )
        return 0;
      break;
      }
    case SST032_JID:
      {
      if( uwReadAddr + uNumBytes > SST032_CAPACITY )
        return 0;
      break;
      }
    case SST064_JID:
      if( uwReadAddr + uNumBytes > SST064_CAPACITY )
        return 0;
      break;

    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uint32_t uCapacity = CAPACITY(m_FlashJID);
      if( uwReadAddr + uNumBytes > uCapacity )
        return 0;
      break;
      }
      
    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
        
      return 0;
      }
    }
    
  SPIFLASH_WAIT_MUTX;
  
  _SPI_Config();   // 配置SPI到适合的工作模式
  
  uint8_t* pucBuf = (uint8_t*)pvBuffer;
  
  ucHead[0] = FLASH_HiREAD;
  ucHead[1] = uwReadAddr >> 16;
  ucHead[2] = uwReadAddr >> 8;
  ucHead[3] = uwReadAddr;
  ucHead[4] = 0;               // a dummy byte

  Set_nCS_Low();

  if( HAL_OK != SPI_Write( ucHead, 5 ) )
    uRes = 0;
  else if( uNumBytes < 0x10000 )
    {
    if( HAL_OK == SPI_Read( pucBuf, uNumBytes ) )
      uRes = uNumBytes;
    else
      uRes = 0;
    }
  else
    {
    uRes = 0;
    
    while( uNumBytes > 0xF000 )
      {
      if( HAL_OK == SPI_Read( pucBuf, 0xF000 ) )
        uRes += 0xF000;
      
      pucBuf    += 0xF000;
      uNumBytes -= 0xF000;
      }
      
    if( uNumBytes )
      if( HAL_OK == SPI_Read( pucBuf, uNumBytes ) )
        uRes += uNumBytes;
    }
  
  Set_nCS_High();
    
  SPIFLASH_RELEASE_MUTX;
  
  return uRes;
}
                   
//-----------------------------------------------------------------------------
// 从指定地址开始读出指定长度的数据         
// 速度25MHz  
uint32_t SPIFLASH_Read25(uint32_t  uwReadAddr,
                             void *pvBuffer,   
                         uint32_t  uNumBytes)
{

  uint32_t uRes;
  uint8_t  ucHead[4]; 

  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  switch( m_FlashJID )
    {
    case SST016_JID:
      {
      if( uwReadAddr + uNumBytes > SST016_CAPACITY )
        return 0;
      break;
      }
    case SST032_JID:
      {
      if( uwReadAddr + uNumBytes > SST032_CAPACITY )
        return 0;
      break;
      }
    case SST064_JID:
      {
      if( uwReadAddr + uNumBytes > SST064_CAPACITY )
        return 0;
      break;
      }
      
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uint32_t uCapacity = CAPACITY(m_FlashJID);
      if( uwReadAddr + uNumBytes > uCapacity )
        return 0;
      break;
      }
    
    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
        
      return 0;
      }
    }
  
  SPIFLASH_WAIT_MUTX;
  
  _SPI_Config();   // 配置SPI到适合的工作模式
  
  uint8_t *pucBuf = (uint8_t*)pvBuffer;
  
  Set_nCS_Low();

  ucHead[0] = FLASH_READ;
  ucHead[1] = uwReadAddr >> 16;
  ucHead[2] = uwReadAddr >> 8;
  ucHead[3] = uwReadAddr;

  if( HAL_OK != SPI_Write( ucHead, 4 ) )
    uRes = 0;
  else if( uNumBytes < 0x10000 )
    {
    if( HAL_OK != SPI_Read( pucBuf, uNumBytes ) )
      uRes = 0;
    else
      uRes = uNumBytes;
    }
  else
    {
    uRes = 0;
    
    while( uNumBytes > 0xF000 )
      {
      if( HAL_OK == SPI_Read( pucBuf, 0xF000 ) )
        uRes += 0xF000;
      
      pucBuf     += 0xF000;
      uNumBytes -= 0xF000;
      }
      
    if( uNumBytes )
      if( HAL_OK == SPI_Read( pucBuf, uNumBytes ) )
        uRes += uNumBytes;
    }
  
  Set_nCS_High();
  
  SPIFLASH_RELEASE_MUTX;

  return uRes;
}                   
//-----------------------------------------------------------------------------
// 从指定扇区读出数据               
uint32_t SPIFLASH_ReadSectors( uint32_t  uSectorNo,
                                     void *pvBuffer, 
                                 uint32_t  uNumSectors)
{

  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  uint32_t uAdress, uNumBytes;
  switch( m_FlashJID )
    {
    case SST016_JID:
      {
      uAdress   = uSectorNo   * SST016_SIZE_SECTOR;
      uNumBytes = uNumSectors * SST016_SIZE_SECTOR;
      break;
      }
    case SST032_JID:
      {
      uAdress   = uSectorNo   * SST032_SIZE_SECTOR;
      uNumBytes = uNumSectors * SST032_SIZE_SECTOR;
      break;
      }
    case SST064_JID:
      {
      uAdress   = uSectorNo   * SST064_SIZE_SECTOR;
      uNumBytes = uNumSectors * SST064_SIZE_SECTOR;
      break;
      }
      
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uAdress   = uSectorNo   * W25QxxJV_SIZE_SECTOR;
      uNumBytes = uNumSectors * W25QxxJV_SIZE_SECTOR;
      break;
      }
      
    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
        
      return 0;
      }
    } 

  return SPIFLASH_Read( uAdress, pvBuffer, uNumBytes );
}                            
//-----------------------------------------------------------------------------
// 向指定地址写入一个字节
// *注意在此前要调用取消写保护,实际写应使用AAI,此函数在AAI中调用，用于写奇数个字节               
static uint32_t _WriteOnByte( uint32_t uWriteAddr,
                              uint32_t uData )
{

  uint32_t uRes, uWait;
  uint8_t  ucHead[5];
  
  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  uint32_t uCapacity;
  switch( m_FlashJID )
    {
    case SST016_JID:
      {
      uCapacity = SST016_CAPACITY;
      break;
      }

    case SST032_JID:
      {
      uCapacity = SST032_CAPACITY;
      break;
      }

    case SST064_JID:
      {
      uCapacity = SST016_CAPACITY;
      break;
      }
      
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uCapacity = CAPACITY(m_FlashJID);
      break;
      }

    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
        
      return 0;
      }
    } 
  
  if( uWriteAddr >= uCapacity )
    return 0;
   
  SPIFLASH_WAIT_MUTX;
  
  Set_nCS_Low();

  ucHead[0] = FLASH_WRITE;
  ucHead[1] = uWriteAddr >> 16;
  ucHead[2] = uWriteAddr >> 8;
  ucHead[3] = uWriteAddr;
  ucHead[4] = uData;
  
  if( HAL_OK != SPI_Write( ucHead, 5 ) )
    uRes = 0;
  else
    uRes = 1;

  Set_nCS_High();
  
  osDelay( 3 );
  
  uWait = 300;
  while( _CheckBusy(0) && --uWait )
    __NOP();
  
  SPIFLASH_RELEASE_MUTX;

  return uRes;
}
//-----------------------------------------------------------------------------
//向指定地址开始写入指定长度的数据  
//-----------------------------------------------------------------------------
// 用于高速页写
static uint32_t _PageWrite(  uint32_t  uWriteAddr,
                           const void *pvBuffer, 
                             uint32_t  uNumBytes)
{
  

  if( uNumBytes > SST064_SIZE_PAGE )
    uNumBytes = SST064_SIZE_PAGE;
  
  if( (uWriteAddr & (SST064_SIZE_PAGE - 1)) + uNumBytes > SST064_SIZE_PAGE )
    uNumBytes = (uWriteAddr & (SST064_SIZE_PAGE - 1)) + uNumBytes - SST064_SIZE_PAGE;
  
  _Send_Cmd( FLASH_WREN );  // 允许写
  
  Set_nCS_Low();
  
  uint8_t  ucHead[6]; 
  ucHead[0] = FLASH_WRITE;
  ucHead[1] = uWriteAddr >> 16;
  ucHead[2] = uWriteAddr >> 8;
  ucHead[3] = uWriteAddr;
    
  if( HAL_OK != SPI_Write( ucHead, 4 ) )
    {
    Set_nCS_High();
    return 0;
    }
    
  if( HAL_OK != SPI_Write( pvBuffer, uNumBytes ) )
    {
    Set_nCS_High();
    return 0;
    }
    
  Set_nCS_High();
    
  osDelay( 3 );
    
  return uNumBytes;
}
//-----------------------------------------------------------------------------
// 基于页写的064写操作
static uint32_t _FastWrite(  uint32_t  uWriteAddr,
                          const void  *pvBuffer, 
                             uint32_t  uNumBytes )
{

  uint32_t uCnt, uWritenCnt;
  
  if( nullptr == pvBuffer || (uWriteAddr >= SST064_CAPACITY || uNumBytes < 1) )
    return 0;
  
  SPIFLASH_WAIT_MUTX;
  
  _SPI_Config();       // 配置SPI到适合的工作模式

  const uint8_t* pucBuf = (const uint8_t*)pvBuffer;
  
  _WriteEnable();
  
  uWritenCnt = 0;
  do
    {
    uCnt = _PageWrite( uWriteAddr, pucBuf, uNumBytes );
    if( uCnt )
      {
      uWritenCnt += uCnt;
        
      uWriteAddr += uCnt;
      pucBuf     += uCnt;
      uNumBytes  -= uCnt;
      }
    else
      break;
    
    } while( uNumBytes > 0 );

  _WriteDisable();
    
  SPIFLASH_RELEASE_MUTX;

  return uWritenCnt;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef SST25_EAAIP
// AAI模式写
// 064以下芯片只支持AAI写
uint32_t SST25_AAI_Write(   uint32_t  uWriteAddr,
                               const void *pvBuffer, 
                                 uint32_t  uNumBytes)
{

  uint32_t uRes = uNumBytes;
  uint32_t uCnt, iWaitCntr;
  uint8_t  ucHead[6]; 

  SPIFLASH_WAIT_MUTX;
  
  _SPI_Config();   // 配置SPI到适合的工作模式

  const uint8_t* pucBuf = (const uint8_t*)pvBuffer;

  _WriteEnable();
  
  if( uNumBytes < 2 )
    {
    _WriteOnByte( uWriteAddr, *pucBuf );  
    }
  else
    {
    uCnt = uNumBytes;
      
    SPIFLASH_EBSY();                // 允许SO输出Busy
    _Send_Cmd(FLASH_WREN);  // 允许写
  
    Set_nCS_Low();
  
    ucHead[0] =  SST25_EAAIP;
    ucHead[1] =  uWriteAddr >> 16;
    ucHead[2] =  uWriteAddr >> 8;
    ucHead[3] =  uWriteAddr;
    ucHead[4] = *pucBuf++;
    ucHead[5] = *pucBuf++;
    
    if( HAL_OK != SPI_Write( ucHead, 6 ) )
      uRes = 0;
    
    Set_nCS_High();
    
    for( iWaitCntr = 10; iWaitCntr; iWaitCntr-- ) 
      __NOP();
    
    Set_nCS_Low();

    iWaitCntr = 0x100;
    while( 0 == SPIFLASH_MISO_STAT && --iWaitCntr ) 
      __NOP();  // Check Busy by SO
    
    Set_nCS_High();
    
    if( 0 == iWaitCntr )  // 
      while( _CheckBusy(0xAA) )
        __NOP();

    uCnt -= 2;  
    for( ; uCnt > 1; uCnt -= 2 )
      {
      Set_nCS_Low();
  
       ucHead[0] = SST25_EAAIP;
      ucHead[1] = *pucBuf++;
      ucHead[2] = *pucBuf++;

      if( HAL_OK != SPI_Write( ucHead, 3 ) )
        uRes = 0;
    
      Set_nCS_High();
    
      for( iWaitCntr = 10; iWaitCntr;  ) 
        iWaitCntr--;
    
      Set_nCS_Low();

      iWaitCntr = 0x100;
      while( 0 == SPIFLASH_MISO_STAT && --iWaitCntr ) 
        __NOP();  // Check Busy by SO
    
      Set_nCS_High();
      
      if( 0 == iWaitCntr )  // 
        while( _CheckBusy(0xAA) )
          __NOP();
      }
    
    // 退出AAI模式
    // WRDI followed by DBSY to exit AAI Mode
    _Send_Cmd(FLASH_WRDI);  
    SPIFLASH_DBSY();
    
    if( uCnt > 0 )
      {
      _WriteEnable();
      _WriteOnByte(uWriteAddr + uNumBytes - 1, *pucBuf );
      }
    }
  
  _WriteDisable();
    
  SPIFLASH_RELEASE_MUTX;
  
  return uRes;
}
#endif
//-----------------------------------------------------------------------------
//向指定地址开始写入指定长度的数据               
uint32_t SPIFLASH_Write( uint32_t  uWriteAddr,
                       const void *pvBuffer, 
                         uint32_t  uNumBytes)
{

  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  uint32_t uRes;
  switch( m_FlashJID )
    {
#ifdef SST25_EAAIP
    case SST016_JID:
      {
      if( uWriteAddr + uNumBytes <= SST016_CAPACITY )
        uRes = SST25_AAI_Write( uWriteAddr, pvBuffer, uNumBytes ); 
      else
        uRes = 0;
      break;
      }
    case SST032_JID:
      {
      if( uWriteAddr + uNumBytes <= SST032_CAPACITY )
        uRes = SST25_AAI_Write( uWriteAddr, pvBuffer, uNumBytes ); 
      else
        uRes = 0;
      break;
      }
#endif // SST25_EAAIP
    case SST064_JID:
      {
      if( uWriteAddr + uNumBytes <= SST064_CAPACITY )
        uRes = _FastWrite( uWriteAddr, pvBuffer, uNumBytes ); 
      else
        uRes = 0;
      break;
      }
      
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uint32_t uCapacity = CAPACITY(m_FlashJID);
      if( uWriteAddr + uNumBytes <= uCapacity )
        uRes = _FastWrite( uWriteAddr, pvBuffer, uNumBytes ); 
      else
        uRes = 0;
      break;
      }
    
    default:
      uRes = 0;
      break;
    } 

  return uRes;    
}
//-----------------------------------------------------------------------------
// 向指定扇区写入数据               
uint32_t SPIFLASH_WriteSectors(   uint32_t  uSectorNo,
                               const void  *pvBuffer, 
                                  uint32_t  uNumSectors)
{
  
  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  uint32_t uRes;
  switch( m_FlashJID )
    {
#ifdef SST25_EAAIP
    case SST016_JID:
      {
      if( uSectorNo + uNumSectors <= SST016_SECTORS )
        uRes = SST25_AAI_Write( uSectorNo * SST016_SIZE_SECTOR, 
                                pvBuffer, 
                                uNumSectors * SST016_SIZE_SECTOR);
      else
        uRes = 0;
      break;
      }
    case SST032_JID:
      {
      if( uSectorNo + uNumSectors <= SST032_SECTORS )
        uRes = SST25_AAI_Write( uSectorNo * SST016_SIZE_SECTOR, 
                                pvBuffer, 
                                uNumSectors * SST032_SIZE_SECTOR);
      else
        uRes = 0;
      break;
      }
#endif
    case SST064_JID:
      {
      if( uSectorNo + uNumSectors <= SST016_SECTORS )
        uRes = _FastWrite( uSectorNo * SST064_SIZE_SECTOR, 
                           pvBuffer, 
                           uNumSectors * SST064_SIZE_SECTOR);
      else
        uRes = 0;
      break;
      }
      
    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uint32_t uCapacity = CAPACITY(m_FlashJID),
               uNumOfSections = W25QxxJV_SECTORS(uCapacity);

      if( uSectorNo + uNumSectors <= uNumOfSections )
        uRes = _FastWrite( uSectorNo * W25QxxJV_SIZE_SECTOR, 
                           pvBuffer, 
                           uNumSectors * W25QxxJV_SIZE_SECTOR);
      else
        uRes = 0;
      break;
      }
    
    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
      uRes = 0; 
      break;
      }
    } 

  return uRes;    
} 
//-----------------------------------------------------------------------------
// 擦除指定扇区               
uint32_t SPIFLASH_EraseSectors(uint32_t uSectorNo, uint32_t uNumSectors )
{

  uint32_t uRes = uNumSectors;
  uint32_t uAddress, uBytesToErase;
  uint8_t  ucCmd[4];
  
  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  uint32_t uNumOfSections, uSizeOfSector;
  switch( m_FlashJID )
    {
#ifdef SST25_EAAIP
    case SST016_JID:
      {
      uNumOfSections = SST016_SECTORS;
      uSizeOfSector  = SST016_SIZE_SECTOR;
      uBytesToErase  = uNumSectors * SST016_SIZE_SECTOR;
      break;
      }
    case SST032_JID:
      {
      uNumOfSections = SST032_SECTORS;
      uSizeOfSector  = SST032_SIZE_SECTOR;
      uBytesToErase  = uNumSectors * SST032_SIZE_SECTOR;
      break;
      }
#endif
    case SST064_JID:
      uNumOfSections = SST064_SECTORS;
      uSizeOfSector  = SST064_SIZE_SECTOR;
      uBytesToErase  = uNumSectors * SST064_SIZE_SECTOR;
      break;

    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      {
      uint32_t uCapacity = CAPACITY(m_FlashJID);
      uNumOfSections = W25QxxJV_SECTORS(uCapacity);
      uSizeOfSector  = W25QxxJV_SIZE_SECTOR;
      uBytesToErase  = uNumSectors * W25QxxJV_SIZE_SECTOR;
      break;
      }
      
    default:
      {
      SetHWFault( RHF_ExFLASH_ERR );
        
      return 0;
      }
    } 
  
  if( uSectorNo + uNumSectors > uNumOfSections )
    return 0;

  SPIFLASH_WAIT_MUTX;
  
  _WriteEnable();

  uAddress = uSectorNo * uSizeOfSector;
  while( uBytesToErase > 0 )
    {
    if( uBytesToErase >= 65536 )
      {
      Set_nCS_Low();
        
      // 64K Bytes Block-Erase
      ucCmd[0] = FLASH_ERASE64;
      ucCmd[1] = uAddress >> 16;
      ucCmd[2] = uAddress >> 8;
      ucCmd[3] = uAddress;
      
      if( HAL_OK != SPI_Write( ucCmd, 4 ) )
        uRes = 0;
      
      Set_nCS_High();
    
      uBytesToErase -= 65536;
      uAddress      += 65536;
      }
    else if( uBytesToErase >= 32768 )
      {
      Set_nCS_Low();
        
      // 32K Bytes Block-Erase
      ucCmd[0] = FLASH_ERASE32;
      ucCmd[1] = uAddress >> 16;
      ucCmd[2] = uAddress >> 8;
      ucCmd[3] = uAddress;
      
      if( HAL_OK != SPI_Write( ucCmd, 4 ) )
        uRes = 0;
      
      Set_nCS_High();
    
      uBytesToErase -= 32768;
      uAddress      += 32768;
      }
    else
      {
      Set_nCS_Low();
        
      // Sector-Erase
      ucCmd[0] = FLASH_ERASE4;
      ucCmd[1] = uAddress >> 16;
      ucCmd[2] = uAddress >> 8;
      ucCmd[3] = uAddress;
      
      if( HAL_OK != SPI_Write( ucCmd, 4 ) )
        uRes = 0;
      
      Set_nCS_High();
    
      uBytesToErase -= uSizeOfSector;
      uAddress      += uSizeOfSector;
      }
      
    osDelay(18);              // Sector-/Block-Erase Time: 18 ms (typical)
    while( _CheckBusy(0) )
      osDelay(1);
    }
 
  _WriteDisable();
      
  SPIFLASH_RELEASE_MUTX;

  return uRes;
}
//-----------------------------------------------------------------------------
// 擦除芯片               
uint32_t SPIFLASH_EraseAll( uint32_t uToken )
{
  
  if( TOKEN_WriteFLASH_Enabled != uToken )
    return 0;

  SPIFLASH_WAIT_MUTX;

  _SPI_Config();             // 配置SPI到适合的工作模式

  _WriteEnable();

  _Send_Cmd(FLASH_ERACHIP);  // Erase all chip

  _WriteDisable();

  osDelay(35);               // Chip-Erase Time: 35 ms(typical) 50ms(Max)

  while( _CheckBusy(0) )
    osDelay(1);

  SPIFLASH_RELEASE_MUTX;

  return 1;
}
//-----------------------------------------------------------------------------
// 读取芯片ID
uint32_t SPIFLASH_GetID(void)
{

  uint32_t uJID;
  uint8_t  uwID[3];
    
  SPIFLASH_WAIT_MUTX;
  
  _SPI_Config();   // 配置SPI到适合的工作模式
  
  Set_nCS_Low();

  uwID[0] = FLASH_RDJID;     // 
  
  if( HAL_OK != SPI_Write( uwID, 1 ) )
    uJID = 0;
  else if( HAL_OK != SPI_Read( uwID, 3 ) )
    uJID = 0;
  else
    uJID = uwID[0] * 0x10000 + uwID[1] * 0x100 + uwID[2];

  Set_nCS_High();
  
  SPIFLASH_RELEASE_MUTX;

  m_FlashJID = uJID;
  
  return uJID;
}
//-----------------------------------------------------------------------------
// 允许/禁止写入
// uEnabled == TOKEN_Write_Enabled时，允许
void SPIFLASH_EnabledWrite( uint32_t uEnabled )
{
  
  if( TOKEN_WriteFLASH_Enabled == uEnabled )
    Set_nWP_High();
  else
    Set_nWP_Low();
}
//-----------------------------------------------------------------------------
// 允许/禁止保持
// uEnabled == TOKEN_Write_Enabled时，允许
void SPIFLASH_EnabledHold( uint32_t uEnabled )
{

  // 
  if( TOKEN_WriteFLASH_Enabled == uEnabled )
    SPIFLASH_nHold_Low();
  else
    SPIFLASH_nHold_High();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//获取芯片参数
void SPIFLASH_GetFlashParam( TFlashParameter *pFP )
{
  
  if( nullptr == pFP )
   return ;
  
  // Wey. 2018.12.11
  // 根据芯片ID区分芯片
  if( !IS_VALID_JID(m_FlashJID) )
    SPIFLASH_GetID();
  
  switch( m_FlashJID )
    {
    case SST016_JID:
      {
      pFP->Type     = SST016_JID;
      pFP->Capacity = SST016_CAPACITY;
      pFP->NumOfSectors = SST016_SECTORS;
      pFP->SectorSize   = SST016_SIZE_SECTOR;
      break;
      }

    case SST032_JID:
      {
      pFP->Type     = SST032_JID;
      pFP->Capacity = SST032_CAPACITY;
      pFP->NumOfSectors = SST032_SECTORS;
      pFP->SectorSize   = SST032_SIZE_SECTOR;
      break;
      }

    case SST064_JID:
      pFP->Type     = SST064_JID;
      pFP->Capacity = SST064_CAPACITY;
      pFP->NumOfSectors = SST064_SECTORS;
      pFP->SectorSize   = SST064_SIZE_SECTOR;
      break;

    case W25Q64JV_JID:
    case W25Q128JV_JID:
    case W25Q256JV_JID:
      pFP->Type     = m_FlashJID;
      pFP->Capacity = CAPACITY(m_FlashJID);
      pFP->NumOfSectors = W25QxxJV_SECTORS(pFP->Capacity);
      pFP->SectorSize   = W25QxxJV_SIZE_SECTOR;
      break;

    default:
      pFP->Type     = 0;
      pFP->Capacity = 0;
      pFP->NumOfSectors = 0;
      pFP->SectorSize   = 0;
    
      SetHWFault( RHF_ExFLASH_ERR );
    
      break;
    } 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static uint8_t pBuf1[4096], pBuf2[4096];
//-----------------------------------------------------------------------------
int DevChk_FlashTest()
{

  uint16_t uwRes = 0;
  
  TFlashParameter flash;
  SPIFLASH_GetFlashParam( &flash );
  uint32_t sSector = flash.SectorSize;
  if( 1024 > sSector )
    return -1;
  
  SPIFLASH_ReadSectors( 33, pBuf1, 1 );

  SPIFLASH_EnabledWrite( TOKEN_WriteFLASH_Enabled );
  SPIFLASH_EraseSectors( 33, 1 );

  SPIFLASH_ReadSectors( 33, pBuf2, 1 );
  for( int iIdx = 0; iIdx < sSector / 4 && uwRes == 0; iIdx++ )
    if( ((uint32_t*)pBuf2)[iIdx] != (uint32_t)-1 )
      uwRes |= 1;

  for( int iIdx = 0; iIdx < sSector; iIdx++ )
    pBuf2[iIdx] = iIdx;

  SPIFLASH_WriteSectors( 33, pBuf2, 1 );
  memset( pBuf2, 0, sSector );
  
  SPIFLASH_ReadSectors( 33, pBuf2, 1 );
  for( int iIdx = 0; iIdx < sSector && uwRes < 2; iIdx++ )
    if( pBuf2[iIdx] != (iIdx & 0xFF) )
      uwRes |= 2;

  SPIFLASH_EraseSectors( 33, 1 );

  SPIFLASH_ReadSectors( 33, pBuf2, 1 );
  for( int iIdx = 0; iIdx < sSector / 4 && uwRes < 4; iIdx++ )
    if( ((uint32_t*)pBuf2)[iIdx] != (uint32_t)-1 )
      uwRes |= 4;

  SPIFLASH_EraseSectors( 33, 1 );
  SPIFLASH_WriteSectors( 33, pBuf1, 1 );  // 写回原内容

  SPIFLASH_ReadSectors( 33, pBuf2, 1 );
  for( int iIdx = 0; iIdx < sSector && uwRes < 8; iIdx++ )
    if( pBuf2[iIdx] != pBuf1[iIdx] )
      uwRes |= 8;

  SPIFLASH_EnabledWrite( 0 );

  return uwRes;
}

//-----------------------------------------------------------------------------
