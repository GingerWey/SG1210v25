//-----------------------------------------------------------------------------
/*
 File        : DevFixed.h
 Version     : V1.10
 By          : 银网科技

 Description :定义装置固化信息结构和管理方法
              1.定义外部NvRom中的空间分配和存贮格式
              2.集中管理访问外部NvRom的方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef DEV_FIXED_H
#define DEV_FIXED_H

#include "Dev_Cfg.h"
#include "DevTypes.h"

#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
// DeviceConfig中的有效性标识
#define DF_VALID        0x5A3C              // 有效性标识
#define DF_INIT         0xA5C3              // 初始值
//-----------------------------------------------------------------------------
// 作业口令
#define TOKEN_FIX_OPERATE  0x3A5CA87C
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------
//  给Boot用的基本网络配置
typedef __PACKED_BEG struct tagBootFixed
{
  uint16_t     Ident;         // 配置标识
  uint8_t      Version;       // 版本
  uint8_t      Length;        // TBootFixed结构的长度
  TETHConfig   ETHConfig;     // 以太网口1的配置
  uint32_t     CRC32;         // 从Ident到Gateway的校验
} __PACKED_END TBootFixed;
//-----------------------------------------------------------------------------
// 设备参数
// 416 x 2 = 832B
typedef __PACKED_BEG struct tagDeviceFixed
{
  uint16_t     Ident;                         // 头标识
  uint16_t     Version;                       // 版本
  uint16_t     Valid;                         // 有效性标识，参数被整定后有效
  uint16_t     Length;                        // 设备配置区的长度
  uint32_t     CRC32;                         // 校验，从HeadRes开始
  uint8_t      DevFunc;                       // 装置功能类型
  uint8_t      ActiveHoldingBlock;            // 当前运行定值区
  uint8_t      Distributor;                   // 装置的经销商
  uint8_t      Language;                      // 界面工作语言 0-中文 1-英文 其它
  uint16_t     Password;                      // 操作口令
  uint16_t     Password2;                     // 二级操作口令
  uint32_t     HeadRes[3];                    // 保留 32字节对齐
  
  uint32_t     Options;                       // 设备配置，按位(bit)使用 
  uint16_t     Configs   [CNT_DevConfigs];    // 设备参数
  uint16_t     CaliCoef  [CNT_Calibration];   // 模拟量校正系数 
                                              // 幅值：calib = (X / x) * 16384

  // 通信配置
#if NUM_ETH_PORTS > 0
  TETHConfig          ETHConfig[NUM_ETH_PORTS];            // 网口配置  0=W5500
  TETHSocketConfig    ETHSocketsConfig[NUM_ETH_SOCKETS];   // 网口1上的 Socket
#endif

#if NUM_UART_PORTS > 0
  TUARTConfig         UARTConfig[NUM_UART_PORTS]; // 串口配置
#endif

#if NUM_CAN_PORTS > 0
  TCANOption          CANConfig[NUM_CAN_PORTS];   // CAN配置
#endif
} __PACKED_END TDeviceFixed;
//-----------------------------------------------------------------------------
#if NUM_ETH_PORTS > 0
  #define  GET_ETHOPT(x)       (&DevConfig.ETHConfig[x])
#endif
//-----------------------------------------------------------------------------
#if NUM_UART_PORTS > 0
  #define  GET_UARTOPT(x)      (&DevConfig.UARTConfig[x])
  #define  GetEditUART1Cfg(x)  (&DevCfgForEdit.UARTConfig[x])
#endif
//-----------------------------------------------------------------------------
#if NUM_CAN_PORTS > 0
  #define  GET_CANOPT(x)       (&DevConfig.CANConfig[x])
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 报告记录
//-----------------------------------------------------------------------------
// 各种记录的索引表
// 20 x 2 = 40B
typedef __PACKED_BEG struct tagLogIndexer
{
  
  uint32_t     uCRC;

  // 事件记录的存贮空间,用于识别不同尺寸的FeRAM
  uint16_t     uwLogSize;
  uint16_t     uwRev;
  
  TRingStore   EventLogs;                   // 事件记录区
  TRingStore   AlarmLogs;                   // 报警记录区
  TRingStore   FaultLogs;                   // 故障记录区
  TRingStore   WaveLogs;                    // 波形记录区
} __PACKED_END TLogIndexer;
//=============================================================================
// 全局数据申明
//-----------------------------------------------------------------------------
// 装置配置数据
extern TDeviceFixed  DevConfig;
// 装置参数区的编辑区
extern TDeviceFixed  DevCfgForEdit;

// 记录索引
extern TLogIndexer   LogIndexer;
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 读运行区定值
// 实始化并装载
void      FIX_Init(void);
// 读取设备参数
uint32_t  FIX_LoadDevConfig(void);
// 保存设备参数
uint32_t  FIX_SaveDevConfig( int iSilent );
// 恢复默认设备参数
void      FIX_LoadDefDevConfig(uint32_t uToken);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 事件记录
//-----------------------------------------------------------------------------
// 读报告记录项数
// 输入：
//     elType: 事件记录的类型
// 返回：
//     指定类型的可用记录数量
uint32_t FIX_ReadEvtLogCount( TEventLogType type );
//-----------------------------------------------------------------------------
// 读报告记录项
// 输入：
//     elType: 事件记录的类型
//    uOffset: 从指定方向偏移量 0...Count-1
//      pvBuf: 保存结果的数据区指针
//   uOptions: 读取记录的方式
//             ERD_FORWARD：  向前，从存贮区中时间最老的记录开始数
//             ERD_BACKWARD:  向后，从存贮区中时间最新的记录开始数
// 返回：
//      0 = 成功
uint32_t FIX_ReadEvtLogItem ( TEventLogType  elType,
                                   uint32_t  uOffset,
                                       void *pvBuf,
                                   uint32_t  uOptions );
//-----------------------------------------------------------------------------
// 保存报告记录项
// 若保存满，则覆盖旧记录项
// 输入：
//     elType: 事件记录的类型
//      pvBuf: 保存记录信息的数据区指针
// 返回：
//      0 = 成功
uint32_t FIX_SaveEvtLogItem ( TEventLogType elType, const void* pvBuf );
//-----------------------------------------------------------------------------
// 清事件记录
// 输入：
//    uToken：令牌
//   elType: 事件类型
// 返回：0=正确执行   >0 失败
uint32_t FIX_ClearEventLog( uint32_t uToken, TEventLogType elType );
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0
// 清波形记录
// 输入：
//    uToken：令牌
// 返回：0=正确执行   >0 失败
uint32_t FIX_ClearWaveLog( uint32_t uToken );
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 保存记录索引
uint32_t FIX_SaveLogIndexer(void);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
