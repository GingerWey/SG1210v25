//-----------------------------------------------------------------------------
/*
   File        : Dev_cfg.h
   Version     : V1.10
   By          : 银网科技

   Description :配置应用的各模块
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H

#include <stdint.h>
//=============================================================================
// 全局宏
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// 各插件板的版本
//----------------------------------------------------------------------------
#define VER_SG12B10                 210     // 主板
#define VER_SG12B20                 210     // 功率板
//-----------------------------------------------------------------------------
// 功能选择
#define SYNC_LOGIC_EN               1       // 同步计算和逻辑
#define SAMPLE_FIR_EN               1       // 使用FIR处理采集点
#define SPECTRUM_EN                 0       // 允许频谱分析
#define WAVELOGGER_EN               0       // 允许录波
//-----------------------------------------------------------------------------
// 模拟采样特性
//-----------------------------------------------------------------------------
// 交流采样通道数量
#define NUM_ADC_CHANNELS            2 

// 每周波采样数
#define NUM_SAMPLES_PER_PEROID     80u
// 交流采样点的数据类型
#define TACSample                   uint16_t

// 用于实时计算的每周波数据点数量
#define SIZE_RTCALC_SAMPLES         (NUM_SAMPLES_PER_PEROID)
#define SIZE_RTCALC_BUFFER          (SIZE_RTCALC_SAMPLES)

// 最大采样计数
#define MAX_SAMPLE_PTR              (NUM_SAMPLES_PER_PEROID) 

#if SPECTRUM_EN > 0
  // 用于频谱分析的每周波数据点数量
  #define NUM_ANALYSER_SAMPLES      (NUM_SAMPLES_PER_PEROID)
  #define SIZE_ANALYSER_BUFFER      (NUM_ANALYSER_SAMPLES)
#endif

#if SAMPLE_FIR_EN > 0
  // FIR长度
  #define SIZE_RT_FIR               12     // 用于实时计算的FIR深度
  #define SIZE_AN_FIR               12     // 用于频谱分析的FIR深度
#else
  #define SIZE_RT_FIR               0      // 用于实时计算的FIR深度
  #define SIZE_AN_FIR               0      // 用于频谱分析的FIR深度
#endif
//-----------------------------------------------------------------------------
// 直流采样通道数量
#define NUM_DCS_CHANNELS            1 
// 直流采样点的数据类型
#define TDCSample                   int16_t
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 设备信息数量
//-----------------------------------------------------------------------------
#define CNT_DevOptions              1      // 设备配置个数，按位计数，按int存贮
 #define CNT_DevOptBits             (CNT_DevOptions * 32)

#define CNT_DevConfigs              32     // 设备参数个数

#define CNT_Calibration             4      // 校准系数个数
//-----------------------------------------------------------------------------
// 装置状态缓冲区 尺寸
#define NUM_DEVICE_STATES           32
// 装置寄存器的数据类型
#define TDevStateReg             uint32_t 

// IO状态缓冲区
#define NUM_IO_STATES            (NUM_DigInputs  + NUM_DigInExts + \
                                  NUM_DigInFuncs + NUM_Relays + 4)
//-----------------------------------------------------------------------------
// 状态寄存器
#define NUM_STATE_REGS              256     //
// 状态寄存器
#define NUM_LOGICSTATE_REGS          32     //
// 状态寄存器的数据类型
#define TStateReg                   uint8_t 

// 通用寄存器长度
#define NUM_COMMON_REGS             128     // 
// 通用寄存器的类型
#define TCommonReg                  uint32_t

// 浮点寄存器长度
#define NUM_REAL_REGS               64     // 
// 通用寄存器的类型
#define TRealReg                    float
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 报告记录 
//-----------------------------------------------------------------------------
// 报告中的数据项数
#define NUM_EVENT_DATA                1
// 事件报告缓冲区中的记录数量
#define SIZE_EVENT_CACHE             32
//-----------------------------------------------------------------------------
// 报告记录最大数量
// 10 x 64 + 10 x 64 + 10 x 128 = 4096B
#define NUM_EVENTLOGS               64      // 事件
#define LEN_EVENT_LOG               10

#define NUM_ALARMLOGS               48      // 告警报文
#define LEN_ALARM_LOG               10

#define NUM_FAULTLOGS               36      // 事故报文
#define LEN_FAULT_LOG               (10 + NUM_EVENT_DATA * sizeof(uint32_t))
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 录波记录
//-----------------------------------------------------------------------------
#define NUM_WAVELOG_CYCLES           20
#define NUM_WAVELOG_PRERECODE         8

// 每通道采样点数
#define NUM_SAMPLOG_PER_PEROID       32
#define NUM_SAMPLOG_PER_CHL   (NUM_SAMPLOG_PER_PEROID * NUM_WAVELOG_CYCLES)
#define SIZE_SAMPLOG_ADCCHL   (NUM_SAMPLOG_PER_CHL)  // 
#define SIZE_WAVELOG_BINCHL   ((NUM_SAMPLOG_PER_CHL + 7) / 8)

// 记录器数量
#define NUM_WAVERECORDER              2
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 各电气量的倍率
//-----------------------------------------------------------------------------
//#define RATIO_Freq                  100
//#define RATIO_Voltage               100
//#define RATIO_Current               100
//#define RATIO_Power                 100
//#define RATIO_Percentage            10000
//#define RATIO_Temperature           10
//#define RATIO_Delay_Second          10      // 0.1s

#define MIN_Volt                    30      //(RATIO_Voltage / 2)
#define MAX_Volt                    600     //(600 * RATIO_Voltage)
#define Clear_LowLimit_En           1
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 开入数量
#define NUM_DigInputs                8      // 内部的开入信号状态
#define NUM_DigInExts                0      // 扩展的开入信号状态
#define NUM_DigInFuncs               0      // 开入的功能状态（不小于别名表数量）
  
// 开入抖延时
// 在采样中断, 实际延时时间 = MIN_DIJEDelay / (NUM_SAMPLES_PER_PEROID * 20)
#define MIN_DIJEDelay               10       
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 输出继电器
//-----------------------------------------------------------------------------
// 输出继电器数量
#define NUM_Relays                   2

// 信号继电器的个数，其返回特性受REG_SRLY_AUTORET控制
#define NUM_Signal_Relays            0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 通讯端口
//-----------------------------------------------------------------------------
//// 网口数量, 及默认配置
#if VER_SG12B10 >= 120
  #define NUM_UART_PORTS              1
#else
  #error 未知的主板版本
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 实时检测 Inspector
#define  AOS_ENABLE                   0

#define  NUM_AOS_REG                  3
#define  SIZE_AOS_BUF               512
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#endif
