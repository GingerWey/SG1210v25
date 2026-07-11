//-----------------------------------------------------------------------------
/*
 File        : DevFixed.cpp
 Version     : V1.10
 By          : 银网科技

 Description :实现装置固化信息结构和管理方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "DevFixed.h"

#ifndef __vmSIMULATOR__
  #include <cmsis_os.h>
#endif

#include "GPVersion.h"
#include "RAMHeap.h"
#include "CRC1632.h"
#include "NvRAM.h"

#include "DevDebug.h"
#include "DevFunc.h"
#include "DevRegs.h"
#include "DevTypes.h"
#include "Dev_Cfg.h"

#if NUM_EVENTLOGS > 0
#include "DevEvtMgr.h"
#endif

#include <string.h>
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
#define NUM_Bytes_SetVal     3
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 固化文件标识
//-----------------------------------------------------------------------------
#define BF_IDENT         0xB7CF              // Boot Config区的标识
#define BF_VERISION      0x0010              // 当前版本
//-----------------------------------------------------------------------------
#define DF_IDENTA        0xDefA              // A区的标识
#define DF_IDENTB        0xDefB              // B区的标识
//-----------------------------------------------------------------------------
// 结构信息版本
#define DF_VERISION110   0x0110              // v1.10
#define DF_VERISION      0x0110              // 当前版本
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 各存贮区的地址定义
//-----------------------------------------------------------------------------
// ------- Boot Config ----------
#define  SIZE_BOOTCFG       ((sizeof(TBootFixed) + 0xF) & ~0xFul)             // 
#define  ADDR_BOOTCFG       (0ul)                                             // 0
  
// ------- DevConfig ----------
#define  SIZE_DEVCFG        ((sizeof(TDeviceFixed) + 0xF) & ~0xFul)           // 0x090
#define  ADDR_DEVCFGA       (SIZE_BOOTCFG + 0ul)                              // 0x020
#define  ADDR_DEVCFGB       (ADDR_DEVCFGA + SIZE_DEVCFG)                      // 0x0B0

#define  DEVCFG_INFO        (&(DevConfig.DevFunc))
#define  SIZE_DEVCFG_HEADER ((uint32_t)DEVCFG_INFO - (uint32_t)&DevConfig)
#define  SIZE_DEVCFG_INFO   (sizeof(TDeviceFixed) - SIZE_DEVCFG_HEADER)       // 

#define  END_OF_DEVCFG      (ADDR_DEVCFGB + SIZE_DEVCFG)                      // 0x140

#ifdef NUM_HoldingBlocks
// ------- DevHolding 定值和压板----------
#define  SIZE_HOLDING       ((sizeof(THoldingSaveBlock) + 0xF) & ~0xF)        // 
#define  SIZE_HOLDING_BLK   (SIZE_HOLDING * 2)                                // 
#define  SIZE_HOLDINGS      (SIZE_HOLDING_BLK * NUM_HoldingBlocks)            // 
#define  ADDR_HOLDING       ((END_OF_DEVCFG + 0x3F) & ~0x3Ful)                // 

#define  ADDR_END_OF_HOLDING  (ADDR_HOLDING + SIZE_HOLDINGS)                  // 
#endif

// ------- Log Indexer ----------
#define  SIZE_LOGINDEXER    ((sizeof(TLogIndexer) + 0xF) & ~0xFul)            // 0x030
#define  ADDR_LOGBEGIN      ADDR_LOGIDXRA
#define  ADDR_LOGIDXRA      (ADDR_LOGIDXRB - SIZE_LOGINDEXER)                 // 0x140
#define  ADDR_LOGIDXRB      (ADDR_EVENTLOG - SIZE_LOGINDEXER)                 // 0x170

#define  LOGIDX_INFO        (void*)(&LogIndexer.uwLogSize)
#define  SIZE_LOGIDX_HEADER ((uint32_t)LOGIDX_INFO - (uint32_t)&LogIndexer)
#define  SIZE_LOGIDX_INFO   (sizeof(TLogIndexer) - SIZE_LOGIDX_HEADER)

// ------- Events Logs 事件记录----------
#define  SIZE_EVENTLOG      (LEN_EVENT_LOG * NUM_EVENTLOGS)                  // 0x280
#define  ADDR_EVENTLOG      ((ADDR_ALARMLOG - SIZE_EVENTLOG) & ~0xF)         // 0x1A0

#define  SIZE_ALARMLOG      (LEN_ALARM_LOG * NUM_ALARMLOGS)                  // 0x1E0
#define  ADDR_ALARMLOG      ((ADDR_FAULTLOG - SIZE_ALARMLOG) & ~0xF)         // 0x420

#define  SIZE_FAULTLOG      (LEN_FAULT_LOG * NUM_FAULTLOGS)                  // 0x1F8
#define  ADDR_FAULTLOG      ((2048 - SIZE_FAULTLOG) & ~0xF)                  // 0x600
#define  ADDR_END_OF_LOG    (ADDR_FAULTLOG + SIZE_FAULTLOG)                  // 0x800

// ------- End of Device Configs ----------
#if defined(NUM_HoldingBlocks)
 #define  END_CONFIG        ADDR_END_OF_HOLDING
#else
 #define  END_CONFIG        END_OF_DEVCFG
#endif

#define  BEGIN_LOGS         ADDR_LOGBEGIN

#define  Size_FeRAM_Free    (BEGIN_LOGS - END_CONFIG)   //

// 事件记录占用的存贮空间
#define  CAPACITY_LOGS      (ADDR_END_OF_LOG - BEGIN_LOGS)
//-----------------------------------------------------------------------------
// 录波波形的长度
#define  SIZE_WavlogItem    ((sizeof(TWaveLogItem) + 0xFFF) & ~0xFFFul)
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
// 给Boot使用的配置
TBootFixed    BootConfig;
//-----------------------------------------------------------------------------
// 装置参数数据
TDeviceFixed  DevConfig;
// 装置参数区的编辑区
TDeviceFixed  DevCfgForEdit;
//-----------------------------------------------------------------------------
#if NUM_HoldingBlocks > 0
// 装置定值运行区
TDevHolding   DevHolding;
// 装置定值编辑区
extern TDevHolding   EditHolding;
#endif
//-----------------------------------------------------------------------------
// 记录索引
TLogIndexer   LogIndexer;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法引用
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#if defined(STM32F103xE)
  #define UID_ADDRESS    0x1FFFF7E8
  #define SIZE_ChipFlash (*(uint16_t*)0x1FFFF7E0 * 1024)
#elif  defined(STM32F427xx) || defined(STM32F429xx) | defined(STM32F407xx)
  #define UID_ADDRESS    0x1FFF7A10
  #define SIZE_ChipFlash (*(uint16_t*)FLASHSIZE_BASE * 1024)
#endif
//-----------------------------------------------------------------------------
// 取设备标识检验
// 输入：
// 返回：设备标识检验
uint32_t GetDevIdentCRC(void)
{
  
#ifndef __vmSIMULATOR__
  uint8_t *pucUID = (uint8_t*)(UID_ADDRESS);
  uint32_t uCRC32 = CRC32( pucUID, 12, (uint32_t)-1 );
#else
    uint32_t uCRC32 = 0xDEADBEEF;
#endif
  
  return uCRC32; 
}
//-----------------------------------------------------------------------------
// Boot-Config
//-----------------------------------------------------------------------------
static uint32_t _LoadBootConfigSection( TBootFixed *pBootCfg )
{
  
  uint32_t uRes = 6;
  
  // 试读5次
  for( uint32_t iIdx = 0; iIdx < 5 && uRes > 0; iIdx++ )
    {
    if( 0 == NvRAM_Read( ADDR_BOOTCFG, pBootCfg, sizeof(TBootFixed) ) )
      {
      if( BF_IDENT    != pBootCfg->Ident   ||
          BF_VERISION != pBootCfg->Version ||
          sizeof(TBootFixed) != pBootCfg->Length )
        uRes = 1;
      else 
        {
        uint32_t uCRC32 = CRC32( pBootCfg, 
                                 sizeof(TBootFixed) - sizeof(uint32_t) );
        if( uCRC32 != pBootCfg->CRC32 )
          uRes = 3;
        else
          uRes = 0;
        }
      }
    else 
      uRes = 5;
    }
    
  return uRes;  
}
//-----------------------------------------------------------------------------
static uint32_t _SaveBootConfig( TBootFixed *pBootCfg )
{
  
  uint32_t uRes = 6;
  
  // 填写配置结构
  pBootCfg->Ident   = BF_IDENT;
  pBootCfg->Version = BF_VERISION;
  pBootCfg->Length  = sizeof(TBootFixed);
  
  uint32_t uCRC32 = CRC32( pBootCfg, 
                           sizeof(TBootFixed) - sizeof(uint32_t) );
  pBootCfg->CRC32 = uCRC32;

  // 试读5次
  for( uint32_t iIdx = 0; iIdx < 5 && uRes > 0; iIdx++ )
    {
    if( 0 == NvRAM_Write( TOKEN_NvRAM_ACCESS, 
                          ADDR_BOOTCFG, 
                          pBootCfg, 
                          sizeof(TBootFixed)) )
      {
      uRes = 0;
      }
    else 
      uRes = 5;
    }
    
  if( 0 == uRes )
    ClrHWFault( RHF_FeRAM_ERR );
  else
    SetHWFault( RHF_FeRAM_ERR );
    
  return uRes;  
}
//-----------------------------------------------------------------------------
static void _ResetBootConfig( TBootFixed *pBootCfg, const TETHConfig* pncETH )
{
  
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pBootCfg || nullptr == pncETH, GFC_EmptyPtr  );
#endif
  
  memcpy( &(pBootCfg->ETHConfig), 
           pncETH,
           sizeof(TETHConfig) );

  _SaveBootConfig( pBootCfg );
}
//-----------------------------------------------------------------------------
static void _LoadBootConfig(void)
{
  
#if NUM_ETH_PORTS > 0
  if( 0 != _LoadBootConfigSection( &BootConfig ) )
    {
    _ResetBootConfig( &BootConfig, DevConfig.ETHConfig );
    }
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Dev-Config
//-----------------------------------------------------------------------------
// 认证v1.50版设备参数区的有效性
static uint32_t _CheckDevConfigV110( )
{
  
  uint32_t uRes = 5;
  if( sizeof(TDeviceFixed) != DevConfig.Length )
    uRes = 3;
  else
    {
    uint32_t uCRC32 = CRC32((void*)DEVCFG_INFO, SIZE_DEVCFG_INFO);
    if( uCRC32 != DevConfig.CRC32 )
      uRes = 4;
    else
      {
      uRes = 0;
      
      DevConfig.Valid = DF_VALID;
      }
    }

  return uRes;
}
//-----------------------------------------------------------------------------
// 读一个设备配置区
static uint32_t _LoadDevConfigSection( uint16_t uwSectIdent )
{
  
  uint32_t uRes = 6;
  uint32_t uAddr;
  
  if( DF_IDENTA == uwSectIdent )
    uAddr = ADDR_DEVCFGA;
  else if( DF_IDENTB == uwSectIdent )  
    uAddr = ADDR_DEVCFGB;
  else
    return 0xFF;
  
  // 试读5次
  for( uint32_t iIdx = 0; iIdx < 5 && uRes > 0; iIdx++ )
    {
    if( 0 == NvRAM_Read( uAddr, &DevConfig, sizeof(DevConfig) ) )
      {
      if( uwSectIdent != DevConfig.Ident )
        uRes = 1;
      else
        {
        // 根据不同版本的配置格式，解析配置信息
        switch( DevConfig.Version )
          {
          case DF_VERISION110:
            {
            uRes = _CheckDevConfigV110();
            break;
            }
          default:
            {
            uRes = 5;
            break;
            }
          }
        }
      }
    }
     
  return uRes;
}
//-----------------------------------------------------------------------------
static uint32_t _SaveDevConfigSection( uint16_t uwSectIdent )
{
  
  uint32_t uAddr;
  
  // A区？
  if( DF_IDENTA == uwSectIdent )
    {
    uAddr = ADDR_DEVCFGA;
    DevCfgForEdit.Ident  = DF_IDENTA;
    }
  else if( DF_IDENTB == uwSectIdent )
    {
    uAddr = ADDR_DEVCFGB;
    DevCfgForEdit.Ident  = DF_IDENTB;
    }
  else
    return 0xFF;
  
  uint32_t uCRC32 = CRC32((void*)&(DevCfgForEdit.DevFunc), SIZE_DEVCFG_INFO);
  DevCfgForEdit.Length  = sizeof(DevConfig);  
  DevCfgForEdit.Version = DF_VERISION;
  DevCfgForEdit.CRC32   = uCRC32;

  uint32_t uRes = 0;
  if( 0 != NvRAM_Write(TOKEN_NvRAM_ACCESS, uAddr, &DevCfgForEdit, sizeof(DevConfig)) )
    uRes = 0x0F; 
    
  // 验证写入结果
  if( 0 == uRes )
    {
    if( 0 == NvRAM_Read( uAddr, &DevCfgForEdit, SIZE_DEVCFG_HEADER ) )
      {
      if( uwSectIdent != DevCfgForEdit.Ident )
        uRes = 1;
      else if( DF_VERISION != DevCfgForEdit.Version )
        uRes = 2;
      else if( sizeof(DevConfig) != DevCfgForEdit.Length )
        uRes = 3;
      else if( uCRC32 != DevCfgForEdit.CRC32 )
        uRes = 4;
      }
    else
      uRes = 5;
    
    if( 0 != uRes )
      {
      // 恢复
      DevCfgForEdit.Ident   = uwSectIdent;
      DevCfgForEdit.Version = DF_VERISION;
      DevCfgForEdit.Length  = sizeof(DevConfig);
      DevCfgForEdit.CRC32   = uCRC32;
      DevCfgForEdit.Valid   = 0;
        
      SetHWFault( RHF_FeRAM_ERR );
      }
    else
      {
      ClrHWFault( RHF_FeRAM_ERR );
      }
    }

  return uRes;
}
//-----------------------------------------------------------------------------
// 读取设备参数默认设置
// 输入：
//   bSave: 是否保存
static uint32_t _ResetDevConfig(bool bSave)
{
  
  memset( (uint8_t*)&DevCfgForEdit, 0, sizeof(DevCfgForEdit) );
    
  // -------- 设备参数 --------
  // 由功能类型填写
  DevFunc_LoadDefDevConfig( &DevCfgForEdit );
  
  // 若原装置参数是有效的，则保留关键配置 Wey. 2018.3.3
  if( DF_VALID == DevConfig.Valid )
    {
    DevCfgForEdit.DevFunc   = DevConfig.DevFunc;
    DevCfgForEdit.Language  = DevConfig.Language;
      
    if( 0 == DevCfgForEdit.Password )   // Wey. 2024.3.31
      {
      DevCfgForEdit.Password  = DevConfig.Password; 
      DevCfgForEdit.Password2 = DevConfig.Password2;
      }
    DevCfgForEdit.Distributor        = DevConfig.Distributor;
    DevCfgForEdit.ActiveHoldingBlock = DevConfig.ActiveHoldingBlock;
    }
  else 
    {
    DevCfgForEdit.Password  = 5555;
    DevCfgForEdit.Password2 = 2599;
    }

  DevCfgForEdit.Valid    = DF_INIT; // 初始状态

  DevCfgForEdit.Configs[REG_AUTOCTRL_EN    - REG_DEVCONFIG] = STATE_TRUE;
  DevCfgForEdit.Configs[REG_AUTO_TURNOFF   - REG_DEVCONFIG] = STATE_TRUE;
  DevCfgForEdit.Configs[REG_ACTION_VOLTAGE - REG_DEVCONFIG] = 80;
  DevCfgForEdit.Configs[REG_COIL_VOLTAGE   - REG_DEVCONFIG] = 220;
  DevCfgForEdit.Configs[REG_PWRON_TIME     - REG_DEVCONFIG] = 10;
  DevCfgForEdit.Configs[REG_PWROFF_TIME    - REG_DEVCONFIG] = 10;
  DevCfgForEdit.Configs[REG_SHUTDOWN_TIME  - REG_DEVCONFIG] = 1;
#ifdef REG_PASSBY_EN
  DevCfgForEdit.Configs[REG_PASSBY_EN      - REG_DEVCONFIG] = STATE_TRUE;
#endif
#ifdef REG_VOLTAGE_LEVEL
  DevCfgForEdit.Configs[REG_VOLTAGE_LEVEL  - REG_DEVCONFIG] = 220;
#endif

#if NUM_ETH_PORTS > 0
  // -------- ETH1 --------
  TETHConfig* pNICCfg;
  pNICCfg = DevCfgForEdit.ETHConfig + 0;
  uint8_t ucIPAddr1[] = DEF_ETH1_IPADDR1;
  uint8_t ucNetMask[] = DEF_ETH1_NETMASK;
  memcpy( pNICCfg->Addr, ucIPAddr1, 4 );
  memcpy( pNICCfg->Mask, ucNetMask, 4 );
#ifdef __vmSIMULATOR__
  uint32_t uCRC = 0x2018E260;
#else
  uint32_t uCRC = GetDevIdentCRC();
#endif
  pNICCfg->MAC[0] = 0x02;
  pNICCfg->MAC[1] = uCRC;
  pNICCfg->MAC[2] = uCRC >> 8;
  pNICCfg->MAC[3] = uCRC >> 16;
  pNICCfg->MAC[4] = uCRC >> 24;
  pNICCfg->MAC[5] = pNICCfg->Addr[3];

  // ETH1 Socket
  for( uint32_t uIdx = 0; uIdx < NUM_ETH_SOCKETS; uIdx++ )
    {
    DevCfgForEdit.ETHSocketsConfig[uIdx].Mode     = SKM_SERVER;
    DevCfgForEdit.ETHSocketsConfig[uIdx].Port     = 5000;
    DevCfgForEdit.ETHSocketsConfig[uIdx].Protocol = 0;
    }
#endif

#if NUM_UART_PORTS > 0
  for( uint32_t uIdx = 0; uIdx < NUM_UART_PORTS; uIdx++ )
    {
    DevCfgForEdit.UARTConfig[uIdx].Addr     = uIdx + 1;
    DevCfgForEdit.UARTConfig[uIdx].Baudrate = 1;   // 9600
    DevCfgForEdit.UARTConfig[uIdx].Parity   = 0;   // No Parity
    DevCfgForEdit.UARTConfig[uIdx].Config   = 0;
    DevCfgForEdit.UARTConfig[uIdx].Protocol = 0;
    DevCfgForEdit.UARTConfig[uIdx].Interval = 100;
    }
#endif

#if NUM_CAN_PORTS > 0
  for( uint32_t uIdx = 0; uIdx < NUM_CAN_PORTS; uIdx++ )
    {
    DevCfgForEdit.CANConfig[uIdx].CanMode  = 0;  // NORMAL
    DevCfgForEdit.CANConfig[uIdx].Mode     = 0;  //
    DevCfgForEdit.CANConfig[uIdx].Baudrate = 4;  // 500k
    DevCfgForEdit.CANConfig[uIdx].Filter   = 0;
    DevCfgForEdit.CANConfig[uIdx].Mask     = 0;
    DevCfgForEdit.CANConfig[uIdx].Protocol = 0;
    DevCfgForEdit.CANConfig[uIdx].Interval = 100;
    DevCfgForEdit.CANConfig[uIdx].Options  = 0;
    }
#endif

  // -------- 保存配置数据 --------
  // 保存
  uint32_t uRes;
  if( true == bSave )
    {
    // 保存到NvRAM
    uRes = FIX_SaveDevConfig( 0 );

#ifdef REG_DEV_MODE
    // 进入“参数设置”模式，禁止保护运行
    TDevStateReg dsOldDevState = GetDevMode;
    SetDevMode( RDM_SETCFG );
#endif

    // 复制到运行区
    memcpy( (uint8_t*)&DevConfig, &DevCfgForEdit, sizeof(DevConfig) );
    if( 0 != uRes )
      {
      DevConfig.Valid = 0;        // 配置参数无效
      
#ifdef REG_DEV_MODE
      // 恢复工作模式
      SetDevMode( dsOldDevState );
#endif
      }
    else
      {
#ifdef REG_DEV_MODE
      // 恢复
      SetDevMode(RDM_INIT);
#endif        

#if NUM_ETH_PORTS > 0
      // 重置Boot参数
      _ResetBootConfig( &BootConfig, DevConfig.ETHConfig );
#endif
      }
    }
  else
    uRes = 0;

  return uRes;
}
//-----------------------------------------------------------------------------
// 读取设备参数
static uint32_t _LoadDevConfig()
{
  
#ifdef REG_DEV_MODE
  // 进入“参数设置”模式，禁止保护运行
  TDevStateReg dsOldDevState = GetDevMode;
  SetDevMode( RDM_SETCFG );
#endif
  
  // 读B区
  uint32_t uStep = 0, uRes = 0;
  if( 0 != _LoadDevConfigSection(DF_IDENTB) )
    {
    // B区失效
    // 读A区
    if( 0 == _LoadDevConfigSection(DF_IDENTA) )
      {
      uStep = 1;

      // 复制到 编辑区
      memcpy( (uint8_t*)&DevCfgForEdit, &DevConfig, sizeof(DevConfig) ); 
        
      // A区有效，用A区覆盖B区
      uRes = _SaveDevConfigSection( DF_IDENTB );
      }
    else
      {
      uStep = 2;

#if NUM_EVENTLOGS > 0
      // 发送事件
      EVTMGR_AppendEvent( REG_EC_DEVCFGERR, STATE_TRUE );
#endif
        
      // 未写过的NvRAM是全FF
//      uint32_t *pDevCfg = (uint32_t*)&DevConfig;
//      if( (0xFFFFFFFF == pDevCfg[0] && 0xFFFFFFFF == pDevCfg[1]) || 
//          (0 == pDevCfg[0] && 0 == pDevCfg[1]) )
        {
        // 初始化存贮区
        if( 0 == _ResetDevConfig( true ) )
          {
          DevConfig.Valid = DF_INIT;

#if NUM_EVENTLOGS > 0
          // 发送事件
          EVTMGR_AppendEvent( REG_EC_DEVCFGINIT, STATE_TRUE );
#endif
          }
        else
          DevConfig.Valid = 0;
        }
// //     else
//        {
//#ifdef REG_DEV_MODE
//        // 置出错标识
//        SetDevMode( RDM_FIXERROR );
//        SetFXState( RFE_DCFG_ERROR );
//#endif        
//        DevConfig.Valid = 0;
//        }
      }
    }
  // 检查A区
  else if( 0 != _LoadDevConfigSection(DF_IDENTA) )
    {
    uStep = 3;

    // 读B区恢复A区
    uRes = _LoadDevConfigSection(DF_IDENTB);
      
    if( 0 == uRes )
      {
      // 复制到 编辑区
      memcpy( (uint8_t*)&DevCfgForEdit, &DevConfig, sizeof(DevConfig) ); 
      
      // 保存
      uRes = _SaveDevConfigSection( DF_IDENTA );
      }

    if( 0 == uRes )
      DevConfig.Valid = DF_VALID;       
    }
  else
    {
    uRes  = 0;
    uStep = 4;
      
    DevConfig.Valid = DF_VALID;
    }
    
  if( DF_VALID == DevConfig.Valid || 
      DF_INIT  == DevConfig.Valid )
    {
#ifdef REG_EDIT_STATE
    // 更新当前编辑区当前运行区
    GetEditBlock = DevConfig.ActiveHoldingBlock;
#endif   
    // 读取并检查Boot配置
    _LoadBootConfig();
    }
    
#ifdef REG_DEV_MODE
  // 恢复装置工作模式
  if( RDM_SETCFG == GetDevMode )
    SetDevMode( dsOldDevState );
#endif
    
  return uStep * 0x1000000 + uRes;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Log-Indexer
//-----------------------------------------------------------------------------
// 读一个记录索引区
static uint32_t _LoadLogIdxSection( uint32_t uwSectIdent )
{
  
  uint32_t uRes = 6;
  uint32_t uAddr ;
  
  if( ADDR_LOGIDXRA == uwSectIdent )
    uAddr = ADDR_LOGIDXRA;
  else if( ADDR_LOGIDXRB == uwSectIdent )  
    uAddr = ADDR_LOGIDXRB;
  else
    return 0xFF;
  
  // 试读5次
  for( uint32_t iIdx = 0; iIdx < 5 && uRes > 0; iIdx++ )
    {
    if( 0 == NvRAM_Read( uAddr, &LogIndexer, sizeof(TLogIndexer) ) )
      {
      uint32_t uCRC32 = CRC32(LOGIDX_INFO, SIZE_LOGIDX_INFO);
      if( uCRC32 != LogIndexer.uCRC )
        uRes = 2;
#ifdef CAPACITY_LOGS
      else if( CAPACITY_LOGS != LogIndexer.uwLogSize )
        uRes = 3;
#endif
      else
        uRes = 0;
      }
    else 
      uRes = 5;
    }
    
  return uRes;
}
//-----------------------------------------------------------------------------
// 存贮一个记录索引区
static uint32_t _SaveLogIdxSection( uint32_t uwSectIdent )
{
  
  uint32_t uAddr;
  
  if( ADDR_LOGIDXRA == uwSectIdent )
    uAddr = ADDR_LOGIDXRA;
  else if( ADDR_LOGIDXRB == uwSectIdent )  
    uAddr = ADDR_LOGIDXRB;
  else
    return 0xFF;
  
  // 填写校验区
  uint32_t uCRC = CRC32(LOGIDX_INFO, SIZE_LOGIDX_INFO);
  LogIndexer.uCRC = uCRC;
 
  // 写入NvRAM
  uint32_t uRes = 0;
  if( 0 != NvRAM_Write( TOKEN_NvRAM_ACCESS,
                         uAddr,
                         &LogIndexer,
                         sizeof(TLogIndexer)) )
    uRes = 0x0F;
  
  // 验证写入结果
  if( 0 == uRes )
    {
    if( 0 == NvRAM_Read( uAddr, &uCRC, sizeof(uCRC) ) )
      {
      if( uCRC != LogIndexer.uCRC )
        uRes = 1;
      }
    else
      uRes = 5;
    }
  
  return uRes;
}
//-----------------------------------------------------------------------------
// 保存记录索引
static uint32_t _saveLogIndexer()
{
  
  uint32_t uRes, uIdx;
  
  for( uIdx = 0; uIdx < 3; uIdx++ )
    {
    uRes = 0;
      
    if( 0 != _SaveLogIdxSection(ADDR_LOGIDXRA) )
      uRes |= 1;
  
    if( 0 != _SaveLogIdxSection(ADDR_LOGIDXRB) )
      uRes |= 2;
    
    if( 0 == uRes )
      break;
    }
    
   return uRes;
}
//-----------------------------------------------------------------------------
static uint32_t _ResetLogIndexer()
{
  
  // 初始化记录索引
  memset( &LogIndexer, 0, sizeof(TLogIndexer) );        
  // 各报告记录区的数量
  LogIndexer.EventLogs.Total   = NUM_EVENTLOGS;
  LogIndexer.EventLogs.ItemLen = LEN_EVENT_LOG;
  LogIndexer.AlarmLogs.Total   = NUM_ALARMLOGS;
  LogIndexer.AlarmLogs.ItemLen = LEN_ALARM_LOG;
  LogIndexer.FaultLogs.Total   = NUM_FAULTLOGS;
  LogIndexer.FaultLogs.ItemLen = LEN_FAULT_LOG;

  // Flash是以4K为单位
  // Total按 使用4MB Flash计算
#if WAVELOGGER_EN > 0
  LogIndexer.WaveLogs.ItemLen  = SIZE_WavlogItem / 1024;
  LogIndexer.WaveLogs.Total    = NvRAM_GetFlashSize() / SIZE_WavlogItem;
#else
  LogIndexer.WaveLogs.ItemLen  = 0;
  LogIndexer.WaveLogs.Total    = 0;
#endif

#ifdef CAPACITY_LOGS
  // 保存事件记录使用的存贮空间
  LogIndexer.uwLogSize = CAPACITY_LOGS;
#else
  // 保存事件记录使用的存贮空间
  LogIndexer.uwLogSize = 0;
#endif

  // 保存
  uint32_t uRes = _saveLogIndexer();

#if NUM_EVENTLOGS > 0
  if( 0 == uRes )
    // 发送事件
    EVTMGR_AppendEvent( REG_EC_EVENTLOGINIT, STATE_TRUE );
#endif
  
  return uRes;
}
//-----------------------------------------------------------------------------
// 读取记录索引
static uint32_t _LoadLogIndexer()
{

  // 读B区
  uint32_t uStep = 0, uRes = 0;
  if( 0 != _LoadLogIdxSection( ADDR_LOGIDXRB ) )
    {
    // B区失效
    // 读A区
    if( 0 == _LoadLogIdxSection( ADDR_LOGIDXRA ) )
      {
      uStep = 1;

      // A区有效，用A区覆盖B区
      uRes = _SaveLogIdxSection( ADDR_LOGIDXRB );
      }
    else
      {
      uStep = 2;

#if NUM_EVENTLOGS > 0
      // 发送事件
      EVTMGR_AppendEvent( REG_EC_EVENTLOGERR, STATE_TRUE );
#endif
        
      _ResetLogIndexer();
      }
    }
  // B区正确，检查A区
  else if( 0 != _LoadLogIdxSection( ADDR_LOGIDXRA ) )
    {
    uStep = 3;

    // 读B区恢复A区
    uRes = _LoadLogIdxSection( ADDR_LOGIDXRB );
    if( 0 == uRes )
      uRes = _SaveLogIdxSection( ADDR_LOGIDXRA );
    }
  else
    {
    uRes  = 0;
    uStep = 4;
    }

  // 定义有变化
  if( LogIndexer.EventLogs.Total   != NUM_EVENTLOGS ||
      LogIndexer.EventLogs.ItemLen != LEN_EVENT_LOG ||
      LogIndexer.AlarmLogs.Total   != NUM_ALARMLOGS ||
      LogIndexer.AlarmLogs.ItemLen != LEN_ALARM_LOG ||
      LogIndexer.FaultLogs.Total   != NUM_FAULTLOGS ||
      LogIndexer.FaultLogs.ItemLen != LEN_FAULT_LOG

#if WAVELOGGER_EN > 0
      || LogIndexer.WaveLogs.ItemLen  != SIZE_WavlogItem / 1024
      || LogIndexer.WaveLogs.Total    >  NvRAM_GetFlashSize() / SIZE_WavlogItem 
#endif
     )
    _ResetLogIndexer();

  return uStep * 0x1000000 + uRes;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 实始化并装载
void FIX_Init(void)
{
  
#ifdef USE_DEV_ASSERT  
  int32_t iFeRAMFeee  = Size_FeRAM_Free,
          iCfgSize    = SIZE_DEVCFG,
          iCfgAddrA   = ADDR_DEVCFGA,
          iCfgAddrB   = ADDR_DEVCFGB,
          iCfgdEnd    = END_CONFIG,
          iLogIdxSize = SIZE_LOGINDEXER,
          iLogStart   = ADDR_LOGBEGIN,
          iLogAddrA   = ADDR_LOGIDXRA,
          iLogAddrB   = ADDR_LOGIDXRB,
          iEvtSize    = SIZE_EVENTLOG,
          iEventAdd   = ADDR_EVENTLOG,
          iAlmSize    = SIZE_ALARMLOG,
          iAlarmAdd   = ADDR_ALARMLOG,
          iFaultSize  = SIZE_FAULTLOG,
          iFaultAdd   = ADDR_FAULTLOG;
  
  // 存贮空间不足
  if( iFeRAMFeee < 0 || iLogStart < iCfgdEnd )
    {
    // 锁死以提醒
    DEV_FAULT(GFC_ErrParam  );
      
    return;
    }
#endif
    
  NvRAM_Init();

  // 读事件记录索引
  _LoadLogIndexer();

  // 读装置配置参数
  _LoadDevConfig();

#ifdef REG_DEV_MODE
  if( RDM_INIT != GetDevMode ) 
    return ;
  else
#endif
    // 复制到编辑区
    memcpy( (uint8_t*)&DevCfgForEdit, &DevConfig, sizeof(DevConfig) );

  // 确保校准系统有效
  if( 0 == DevCfgForEdit.CaliCoef[0] )
    {
    const TDevFuncInterface* pFunc = DevFunc_CurFuncInterface();
    if( nullptr != pFunc && nullptr != pFunc->LoadDefDevConfig )
      {
      pFunc->LoadDefDevConfig( &DevCfgForEdit );
      
      FIX_SaveDevConfig( 1 );

      memcpy( (uint8_t*)&DevConfig, &DevCfgForEdit, sizeof(DevConfig) );
      }
    }

#if NUM_HoldingBlocks > 0
  // 读定值和压板
  FIX_LoadHolding();
  
  // 启动时，编辑区就是运行区；
  // 所以读回运行区，复制到编辑区即可
  memcpy( &EditHolding, &DevHolding, sizeof(EditHolding) );
#endif
  
  // Flash
  NvRAM_InitFlash();
}
//-----------------------------------------------------------------------------
// 读取设备参数
uint32_t FIX_LoadDevConfig()
{
  
#ifdef REG_DEV_MODE
  TDevStateReg dsOldDevState = GetDevMode;
  SetDevMode ( RDM_SETCFG );
#endif
  
  uint32_t uRes = _LoadDevConfig();

#ifdef REG_DEV_MODE
  // 恢复装置工作模式
  if( RDM_SETCFG == GetDevMode )
    SetDevMode( dsOldDevState );
#endif
  
  return uRes;
}
//-----------------------------------------------------------------------------
// 保存设备参数
uint32_t FIX_SaveDevConfig( int iSilent )
{
  
  uint32_t uRes, uIdx;

  for( uIdx = 0; uIdx < 3; uIdx++ )
    {
    uRes = 0;
      
    if( 0 != _SaveDevConfigSection( DF_IDENTA ) )
      uRes |= 1;
  
    if( 0 != _SaveDevConfigSection( DF_IDENTB ) )
      uRes |= 2;
    
    if( 0 == uRes )
      break;
    }
    
  // 存设备参数失败
  if( uIdx >= 3 )
    {
#ifdef REG_DEV_MODE
    // 硬件错误
    SetDevMode( RDM_HWFAULT );
    SetHWFault( RHF_FeRAM_ERR );
#endif
      
#if NUM_EVENTLOGS > 0
    if( 0 == iSilent )
      // 发送事件
      EVTMGR_AppendEvent( REG_EH_FeRAM_FAULT, STATE_TRUE );
#endif
    }
  else
    {
#if NUM_ETH_PORTS > 0
    // 更新Boot参数
    _ResetBootConfig( &BootConfig, DevCfgForEdit.ETHConfig );
#endif
    }

  return uRes;  
}
//-----------------------------------------------------------------------------
// 恢复默认设备参数
void FIX_LoadDefDevConfig(uint32_t uToken)
{

#ifdef USE_DEV_ASSERT
  // 令牌不正确
  DEV_ASSERT( TOKEN_FIX_OPERATE != uToken, GFC_ErrToken );
#else  
  if( TOKEN_FIX_OPERATE != uToken )
    return ;
#endif

  // 读取默认参数，但不保存
  _ResetDevConfig( false );

  // 当前设置有效时，保留部分配置
  if( DF_VALID == DevConfig.Valid )
    {
    // 设备功能不变
    DevCfgForEdit.DevFunc = DevConfig.DevFunc;
    DevCfgForEdit.Distributor = DevConfig.Distributor;
    DevCfgForEdit.ActiveHoldingBlock = DevConfig.ActiveHoldingBlock;

    // 保留口令
    DevCfgForEdit.Password  = DevConfig.Password;
    DevCfgForEdit.Password2 = DevConfig.Password2;

//    // 保留网络配置
//    memcpy( (void*)&DevCfgForEdit.ETHConfig,
//            (void*)&DevConfig.ETHConfig,
//            sizeof(TETHConfig) );

    memcpy( (void*)DevCfgForEdit.CaliCoef,
            (void*)DevConfig.CaliCoef,
            sizeof( DevConfig.CaliCoef ) );
    }

  // 保存到NvRAM
  uint32_t uRes = FIX_SaveDevConfig( 0 );
  if( 0 == uRes )
    {
    DevCfgForEdit.Valid = DF_VALID;

#ifdef REG_DEV_MODE
    // 进入“参数设置”模式，禁止保护运行
    TDevStateReg dsOldDevState = GetDevMode;
    SetDevMode( RDM_SETCFG );
#endif
      
    // 复制到运行区
    memcpy( (uint8_t*)&DevConfig, &DevCfgForEdit, sizeof(DevConfig) );
      
#ifdef REG_DEV_MODE
    // 恢复工作模式
    SetDevMode( dsOldDevState );
#endif
      
#if NUM_EVENTLOGS > 0
    // 发送事件
    EVTMGR_AppendEvent( REG_EC_DEVCFGINIT, STATE_TRUE );
#endif
    }
  else
    DevCfgForEdit.Valid = 0;

#ifdef REG_DEV_MODE
  // 写寄存器可能会引起标志设置
  ClrGUIState( REM_CFG_MODIFIED | REM_UART1_MODIFIED );//| REM_CMX_MODIFIED );
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 报告记录读写
//-----------------------------------------------------------------------------
// 读报告记录项数
uint32_t FIX_ReadEvtLogCount( TEventLogType type )
{

  if( DF_VALID != DevConfig.Valid )
    return 0;

  uint32_t uRes;
  switch( type )
    {
    case mltDeviceLog:
      {
      uRes = LogIndexer.EventLogs.Count;
      break;
      }
    case mltDevStatus:
      {
      uRes = LogIndexer.AlarmLogs.Count;
      break;
      }
    case mltAutoCtrl:
      {
      uRes = LogIndexer.FaultLogs.Count;
      break;
      }
    default:
      {
      uRes = 0;
      break;
      }
    }

  return uRes;
}
//-----------------------------------------------------------------------------
// 读报告记录项
//    type: 需要读取的记录类型
// uOffset: 偏移的记录数量  偏移方向根据uOption确定
//   pvBuf: 用于填写读取结果的空间指针
// uOption: 搜索方式  ERD_FORWARD=从最老的记录开始向前 ERD_BACKWARD=从最新的记录向后搜索
uint32_t FIX_ReadEvtLogItem ( TEventLogType  type,
                                   uint32_t  uOffset,
                                       void *pvBuf,
                                   uint32_t  uOption )
{

#ifdef USE_DEV_ASSERT
    // 空指针
    DEV_ASSERT( (nullptr == pvBuf), GFC_EmptyPtr  );
#endif

  uint32_t    uAddr    = 0;
  TRingStore *pIndexer = nullptr;

  // 根据记录类型取地址和索引
  switch( type )
    {
    case mltDeviceLog:
      {
      pIndexer = &LogIndexer.EventLogs;
      uAddr    = ADDR_EVENTLOG;

      if( LEN_EVENT_LOG != pIndexer->ItemLen )
        {
        pIndexer->ItemLen = LEN_EVENT_LOG;
        pIndexer->Total   = NUM_EVENTLOGS;
        }

      break;
      }
    case mltDevStatus:
      {
      pIndexer = &LogIndexer.AlarmLogs;
      uAddr    = ADDR_ALARMLOG;

      if( LEN_ALARM_LOG != pIndexer->ItemLen )
        {
        pIndexer->ItemLen = LEN_ALARM_LOG;
        pIndexer->Total   = NUM_ALARMLOGS;
        }

      break;
      }
    case mltAutoCtrl:
      {
      pIndexer = &LogIndexer.FaultLogs;
      uAddr    = ADDR_FAULTLOG;

      if( LEN_FAULT_LOG != pIndexer->ItemLen )
        {
        pIndexer->ItemLen = LEN_FAULT_LOG;
        pIndexer->Total   = NUM_FAULTLOGS;
        }

      break;
      }
    default:
      {
#ifdef USE_DEV_ASSERT
      DEV_ASSERT( true, GFC_ErrParam );
#endif
      pIndexer = nullptr;
      break;
      }
    }

  // 给定的类型不合法
  if( nullptr == pIndexer )
    return 2;

  // 超界
  if( uOffset >= pIndexer->Count )
    return 3;

  uint32_t uPosition;
  // 根据搜索方向，计算读取位置
  if( ERD_BACKWARD == (uOption & ERD_BACKWARD) )
    {
    // 从当前位置向后搜索
    uPosition = pIndexer->Position;

    if( uPosition >= uOffset )
      uPosition -= uOffset;
    else
      uPosition = pIndexer->Count - (uOffset - uPosition);
    }
  else  // forward
    {
    // 从最老的记录向前搜索
    if( pIndexer->Count >= pIndexer->Total )
      {
      uPosition = pIndexer->Position + 1;
      if( uPosition >= pIndexer->Total )
        uPosition = 0;
      }
    else
      uPosition = 0;

    if( uPosition + uOffset < pIndexer->Count )
      uPosition += uOffset;
    else
      uPosition  = (uPosition + uOffset) - pIndexer->Count;
    }

  // Item's Address & Length
  uAddr += uPosition * pIndexer->ItemLen;

  // 读记录
  if( 0 != NvRAM_Read( uAddr, pvBuf, pIndexer->ItemLen ) )
    return 5;

  return 0;
}
//-----------------------------------------------------------------------------
// 保存报告记录项
uint32_t FIX_SaveEvtLogItem ( TEventLogType type, const void* pvBuf )
{

#ifdef USE_DEV_ASSERT
    // 空指针
    DEV_ASSERT( (nullptr == pvBuf), GFC_EmptyPtr  );
#endif

  uint32_t    uAddr   = 0;
  TRingStore *pIndexer = nullptr;

  // 根据记录类型取地址和索引
  switch( type )
    {
    case mltDeviceLog:
      {
      pIndexer = &LogIndexer.EventLogs;
      uAddr    = ADDR_EVENTLOG;

      if( pIndexer->ItemLen != LEN_EVENT_LOG )
        {
        pIndexer->ItemLen = LEN_EVENT_LOG;
        pIndexer->Total   = NUM_EVENTLOGS;
        }

      break;
      }
    case mltDevStatus:
      {
      pIndexer = &LogIndexer.AlarmLogs;
      uAddr    = ADDR_ALARMLOG;

      if( pIndexer->ItemLen != LEN_ALARM_LOG )
        {
        pIndexer->ItemLen = LEN_ALARM_LOG;
        pIndexer->Total   = NUM_ALARMLOGS;
        }

      break;
      }
    case mltAutoCtrl:
      {
      pIndexer = &LogIndexer.FaultLogs;
      uAddr    = ADDR_FAULTLOG;

      if( pIndexer->ItemLen != LEN_FAULT_LOG )
        {
        pIndexer->ItemLen = LEN_FAULT_LOG;
        pIndexer->Total   = NUM_FAULTLOGS;
        }

      break;
      }
    default:
      {
      pIndexer = nullptr;
      break;
      }
    }

  // 给定的类型不合法
  if( nullptr == pIndexer )
    return 3;

  // 计算存贮位置
  uint32_t uPosition;
  if( pIndexer->Count > 0 )
    {
    if( pIndexer->Position + 1 < pIndexer->Total )
      uPosition = pIndexer->Position + 1;
    else
      uPosition = 0;
    }
  else
    uPosition = 0;

  // Item's Address & Length
  uAddr += uPosition * pIndexer->ItemLen;

  if( 0 != NvRAM_Write(TOKEN_NvRAM_ACCESS, uAddr, pvBuf, pIndexer->ItemLen) )
    return 5;

  // Update Position
  pIndexer->Position = (uint16_t)uPosition;
  if( pIndexer->Total > pIndexer->Count )
    pIndexer->Count++;

  // 保存到NvRAM
  uint32_t uRes = _saveLogIndexer();

  return uRes;
}
//-----------------------------------------------------------------------------
// 清事件记录
// 输入：
//    uToken：令牌
//   elType: 事件类型
// 返回：0=正确执行   >0 失败
uint32_t FIX_ClearEventLog( uint32_t uToken, TEventLogType elType )
{

#ifdef USE_DEV_ASSERT
  // 令牌错误
  DEV_ASSERT( (TOKEN_FIX_OPERATE != uToken), GFC_ErrToken  );
#endif

  TRingStore *pIndexer;
  switch( elType )
    {
    case mltDeviceLog:
      {
      pIndexer = &LogIndexer.EventLogs;
      if( pIndexer->ItemLen != LEN_EVENT_LOG )
        {
        pIndexer->ItemLen = LEN_EVENT_LOG;
        pIndexer->Total   = NUM_EVENTLOGS;
        }
      break;
      }
    case mltDevStatus:
      {
      pIndexer = &LogIndexer.AlarmLogs;
      if( pIndexer->ItemLen != LEN_ALARM_LOG )
        {
        pIndexer->ItemLen = LEN_ALARM_LOG;
        pIndexer->Total   = NUM_ALARMLOGS;
        }
      break;
      }
    case mltAutoCtrl:
      {
      pIndexer = &LogIndexer.FaultLogs;
      if( pIndexer->ItemLen != LEN_FAULT_LOG )
        {
        pIndexer->ItemLen = LEN_FAULT_LOG;
        pIndexer->Total   = NUM_FAULTLOGS;
        }
      break;
      }
    default:
      {
      pIndexer = nullptr;
      break;
      }
    }

  if( nullptr == pIndexer )
    return 82;

  pIndexer->Count = 0;
  pIndexer->Position = 0;

  // 保存到NvRAM
  uint32_t uRes = _saveLogIndexer();

  return uRes;
}
//-----------------------------------------------------------------------------
// 清波形记录
// 输入：
//    uToken：令牌
// 返回：0=正确执行   >0 失败
#if WAVELOGGER_EN > 0
uint32_t FIX_ClearWaveLog( uint32_t uToken )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_FIX_OPERATE != uToken, GFC_ErrToken );
#endif
  
  TRingStore *pIndexer = &LogIndexer.WaveLogs;
  pIndexer->ItemLen  = 0;
  pIndexer->Total    = 0;
  pIndexer->Count    = 0;
  pIndexer->Position = 0;

  // 保存到NvRAM  
  uint32_t uRes = _saveLogIndexer();
  
  return uRes;
}
#endif
//-----------------------------------------------------------------------------
// 保存记录索引
uint32_t FIX_SaveLogIndexer(void)
{
  
  return _saveLogIndexer();
}
//-----------------------------------------------------------------------------

