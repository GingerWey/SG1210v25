//-----------------------------------------------------------------------------
/*
 File        : DevTypes.h
 Version     : V1.10
 By          : 银网科技

 Description :定义系统中公用的数据结构和类型
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef DEV_TYPES_H
#define DEV_TYPES_H

#include "Dev_Cfg.h"

#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
// 状态值
//-----------------------------------------------------------------------------
#define  STATE_TRUE         0xA5
#define  STATE_FALSE        0x00
//-----------------------------------------------------------------------------
// 编译器兼容
#ifdef __SIMULATOR__
   #undef  __PACKED_BEG
   #define __PACKED_BEG
   #undef  __PACKED_END
   #define __PACKED_END
// ARM Compiler V6?
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION > 601050)
   #undef  __PACKED_BEG
   #define __PACKED_BEG  
   #undef  __PACKED_END
   #define __PACKED_END  __attribute__((__packed__))
//GCC compiler?
#elif defined(__GNUC__)
   #undef  __PACKED_BEG
   #define __PACKED_BEG  
   #undef  __PACKED_END
   #define __PACKED_END  __attribute__((__packed__))
//Keil MDK-ARM compiler?
#elif defined(__CC_ARM)
   #undef  __PACKED_BEG
   #define __PACKED_BEG  
   #undef  __PACKED_END
   #define __PACKED_END  __attribute__((__packed__))
//IAR compiler?
#elif defined(__IAR_SYSTEMS_ICC__)
   #undef  __PACKED_BEG
   #define __PACKED_BEG  __packed
   #undef  __PACKED_END
   #define __PACKED_END
//CodeWarrior compiler?
#elif defined(__CWCC__)
   #undef  __PACKED_BEG
   #define __PACKED_BEG
   #undef  __PACKED_END
   #define __PACKED_END
//TI ARM compiler?
#elif defined(tagI_ARM__)
   #undef  __PACKED_BEG
   #define __PACKED_BEG
   #undef  __PACKED_END
   #define __PACKED_END  __attribute__((__packed__))
//Win32 compiler?
#elif defined(_WIN32) || defined(_WIN64) || defined(__vmSIMULATOR__)
   #undef  __PACKED_BEG
   #define __PACKED_BEG
   #undef  __PACKED_END
   #define __PACKED_END
#endif
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------
#define NUM_ADC_Conversions         16
//-----------------------------------------------------------------------------
//                        ADC采样 配置
//-----------------------------------------------------------------------------
// ADC的采样通道定义
typedef struct tagADC3Ranks
{
  uint8_t        adc1[NUM_ADC_Conversions]; // 16是每轮最多通道数量（硬件决定）
  uint8_t        adc2[NUM_ADC_Conversions];
  uint8_t        adc3[NUM_ADC_Conversions];
} TADC3Ranks;
//-----------------------------------------------------------------------------
typedef struct tagADCRanksDef
{
  uint8_t        NbrOfSqs;       // 采样轮次数量
  uint8_t        NbrOfRanks;     // 每轮次中的通道数据

  const TADC3Ranks  *Ranks;

        uint8_t  NbrOfSamp[NUM_ADC_CHANNELS];  // 每个通道的过采样次数
                                               // 值是 0 ~ 18 (STM32)
                                               //      0 = 通道无输入
                                               //   0x80 = 最高位是1，=直流
        uint32_t RefRegNum[NUM_ADC_CHANNELS];  // 每个通道对应的有效值寄存器
} TADCRanksDef;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        频谱相关 
//-----------------------------------------------------------------------------
// 浮点型向量
typedef __PACKED_BEG struct tagFloatVector
{
  float        Modulus;
  float        Angle;
  float        Real;
  float        Image;
} __PACKED_END TFloatVector;
//-----------------------------------------------------------------------------
// 交流频谱结构
typedef __PACKED_BEG struct tagACSpectrum
{
  // 基波的复数和矢量
  __PACKED_BEG struct tagACBase
    {
    // 复数结果
    float      Real;
    float      Image;

    // 向量形式
    float      Modulus;
    float      Angle;
    } __PACKED_END Base
#if SPECTRUM_EN > 0
      ,Harmonic[MAX_HARMONIC_ORDER - 1];
#else
      ;
#endif
} __PACKED_END TACSpectrum;
//-----------------------------------------------------------------------------
// 谐波分析结果数据区
typedef __PACKED_BEG struct tagHarmonicResult
{
  float           rTHD;                     // 总谐波含量(%)
  float           rTotal;                   // 谐波总量
   __PACKED_BEG struct tagHarmDeatil
   {
     float        rValue;                   // 谐波大小
     float        rPrercent;                // 谐波含比
   } __PACKED_END Detail[18];

} __PACKED_END THarmonicResult;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        通信配置相关 
//-----------------------------------------------------------------------------
// Socket.Mode
#define  SKM_IDLE           0
#define  SKM_SERVER         1
#define  SKM_CLIENT         2
#define  SKM_UDP            3
#define  SKM_UDPDGRAM       4
//-----------------------------------------------------------------------------
// 以太网参数
// 18Bytes
typedef __PACKED_BEG struct tagETHConfig
{
  uint8_t          Addr[4];      // 本机地址
  uint8_t          Mask[4];
  uint8_t          Gate[4];
  uint8_t          MAC[6];
} __PACKED_END TETHConfig;
//-----------------------------------------------------------------------------
// 以太网端口参数
// 8字节
typedef __PACKED_BEG struct tagETHSocketConfig
{
  uint8_t          Addr[4];      // 对方地址
  uint16_t         Port;         // 工作端口号
  uint8_t          Mode;         // 工作模式  SKM_xxx宏
  uint8_t          Protocol;     // 通信协议
} __PACKED_END TETHSocketConfig;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// 串口参数
// 4Bytes
typedef __PACKED_BEG struct tagUARTConfig
{
  uint8_t          Addr;         // 1~252
  uint8_t          Protocol;     // 通信协议
  uint8_t          Config;       // UCFG_xxx宏定义的配置信息
  uint32_t         Baudrate;     // 4800-10500000
  uint16_t         Interval;     // 发送间隔
  uint16_t         Parity;       // 校验方式
} __PACKED_END TUARTConfig;
//-----------------------------------------------------------------------------
// 串口配置宏
// TUARTConfig.Mode
// Parity
#define  UCFG_PARITY_NONE         0x00
#define  UCFG_PARITY_EVEN         0x01
#define  UCFG_PARITY_ODD          0x02
#define  UCFG_PARITY_MASK         0x03
#define  UCFG_PARITY_SHIFT        0x00
// DataBits
#define  UCFG_WORDLENGTH_8B       0x00
#define  UCFG_WORDLENGTH_9B       0x04
#define  UCFG_WORDLENGTH_SHIFT    0x02
// StopBits
#define  UCFG_STOPBITS_1          0x00
#define  UCFG_STOPBITS_2          0x08
#define  UCFG_STOPBITS_MASK       0x08
#define  UCFG_STOPBITS_SHIFT      0x03
// FlowCtrl
#define  UCFG_HWCONTROL_NONE      0x00
#define  UCFG_HWCONTROL_RTS       0x10
#define  UCFG_HWCONTROL_CTS       0x20
#define  UCFG_HWCONTROL_MASK      0x30
#define  UCFG_HWCONTROL_SHIFT     0x04
// Mode
#define  UCFG_MODE_RXTX           0x00
#define  UCFG_MODE_RXONLY         0x40
#define  UCFG_MODE_MASK           0x40
#define  UCFG_MODE_SHIFT          0x06

// OverSample
#define  UCFG_OVERSAM_16          0x00
#define  UCFG_OVERSAM_8           0x80
#define  UCFG_OVERSAM_MASK        0x80
#define  UCFG_OVERSAM_SHIFT       0x07

// Options
#define  UCFG_OP_DUPLEX           0x0001
   #define UCFG_OP_HALFDULPLEX    0x0000
   #define UCFG_OP_FULLDULPLEX    0x0001
   #define UCFG_OP_RXONLY         0x0002
   #define UCFG_OP_IREN           0x0010  // 红外模式
   #define UCFG_OP_BDREV          0x0020  //
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// CAN的配置数据
typedef struct tagCANOption
{
  uint8_t          CanMode;       // CAN工作模式 0-休息  1-Host  2-Client
  uint8_t          Mode;          // 工作模式：
  uint8_t          Protocol;      // 通讯协议
  uint8_t          Baudrate;      // 波特率：  
  uint32_t         Filter;        // 
  uint32_t         Mask;          // 
  uint16_t         Interval;      // 轮巡间隔
  uint16_t         Options;       // 配置项
} TCANOption;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        寄存器属性相关 
//-----------------------------------------------------------------------------
// 寄存器描述项
typedef struct tagDevRegInfoItem
{
  uint16_t         NameStrId;     // 名称 Ident
  const char*      pName;         // 名称
  uint32_t         RegNum;        // 寄存器号
  uint32_t         RefReg;        // 参考寄存器
  uint32_t         DefValue;      // 默认值
   int32_t         Min;           // 最小值
  uint32_t         Max;           // 最大值
  uint32_t         Step;          // 编辑步长
   int16_t         Scale;         // 倍率  >0缩小   <0放大
   uint8_t         Width;         // 显示宽度(数字位数，含小数点)
   uint8_t         Decimal;       // 小数位数
   uint8_t         Event;         // 发生事件存贮现场数据的描述项序号
  uint16_t         Misc;          // 其它功能属性，列表中根据应用定义
  uint32_t         Property;      // 属性
} TDevRegInfoItem;
//-----------------------------------------------------------------------------
// 寄存器描述项
typedef struct tagDevRegInfoList
{
  uint32_t         RegBeg,        // 起始寄存器号 
                   RegEnd;        // 结束寄存器号 要求BegReg与RegBeg同类
  uint16_t         Count;         // 列表中描述项的数据
  const TDevRegInfoItem *List;    // 描述项
} TDevRegInfoList;

typedef const TDevRegInfoList*     TConstDevRegInfoListPtr;
//-----------------------------------------------------------------------------
// 寄存器信息列表分组
// TDevFuncInterface.InfoLists 按下列顺序组织，没可填零
// 按使用频度优化
typedef enum tagDevRegListClass
{
  rcProtMeasure      = 1,         // 保护测量
  rcACMeasure        = 2,         // 交流测量
  rcDCMeasure        = 3,         // 直流测量
  rcHarmonic         = 4,         // 谐波统计
  rcBinaryIn         = 5,         // 开入信号
  rcBinInFunc        = 6,         // 开入功能
  rcBinaryOut        = 7,         // 开出状态
  rcProtSignals      = 8,         // 保护状态

  rcEvents           = 9,         // 事件类型
  rcDevOption        = 10,        // 设备配置
  rcDevConfig        = 11,        // 设备配置
  rcDIAlais          = 12,        // 开入别名定义寄存器
  rcProtEnables      = 13,        // 保护压板
  rcProtSet          = 14,        // 保护定值
  rcFunction         = 15,        // 功能寄存器
  rcSysState         = 16,        // 系统状态

  rcMeasAdjust       = 17,        // 模拟手调 
  rcMeasTest         = 18,        // 模拟测试     
  rcDITest           = 19,        // 开入测试
  rcDOTest           = 20         // 开出测试
} TDevRegListClass;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 报告描述
//-----------------------------------------------------------------------------
// 报告属性的ID
#define  EVTP_OPERATE       0x01
#define  EVTP_CFGFAULT      0x02
#define  EVTP_HWFAULT       0x03
#define  EVTP_PROSWTH       0x04      // 保护软压板
#define  EVTP_DIGINPT       0x05
#define  EVTP_RELAY         0x06      // 继电器信号
#define  EVTP_PERIPH        0x07      // 装置外设
#define  EVTP_PROTECT1      0x08
//-----------------------------------------------------------------------------
// 报告记录类型
typedef enum tagEventLogType
{
  mltNew         = 0,       // 最新的事件 
  mltDeviceLog,             // 事件报告
  mltDevStatus,             // 告警
  mltAutoCtrl,              // 事故
//  mltCurProt,             // 电流故障
//  mltRmtCtrl,             // 遥控
//  mltLogic                // 逻辑事件
} TEventLogType;
//-----------------------------------------------------------------------------
// 报告的弹窗和存贮特性
#define EDS_Disp(x)         ((x) & 0x0f)
#define EDS_DISP01          0x01     // 0->1弹窗
#define EDS_DISP10          0x02     // 1->0弹窗
#define EDS_DISPBOTH        0x03     // 1->0 1->0弹窗

#define EDS_Save(x)         ((x) & 0xf0)
#define EDS_SAVE01          0x10     // 0->1保存
#define EDS_SAVE10          0x20     // 1->0保存
#define EDS_SAVEBOTH        0x30     // 1->0 1->0保存
//-----------------------------------------------------------------------------
// 用于报告记录摘要
// 10 Bytes
typedef __PACKED_BEG struct tagEventLogSummary
{
  // 日期
  __PACKED_BEG struct
    {
      unsigned    Year    : 12;
      unsigned    Month   :  4;
      unsigned    Day     :  5;

      unsigned    Hours   :  5;
      unsigned    Minutes :  6;
      unsigned    Seconds :  6;
      unsigned    milSecs : 10;
    } __PACKED_END Time;

  // 描述
  __PACKED_BEG struct
    {
      unsigned    RegNum : 24;       // 报告源寄存器
      unsigned    Action :  4;       // 状态 TRUE = 0->1;  FALSE = 1->0
      unsigned    Type   :  4;       // 故障类型 
    } __PACKED_END State;

} __PACKED_END TEventLogSummary;  
//-----------------------------------------------------------------------------
// 完成的报告记录形式
typedef __PACKED_BEG struct tagEventLogItem
{
  
  TEventLogSummary Summary;                       // 10 Bytes
#if NUM_EVENT_DATA > 0
  uint32_t        FieldData[NUM_EVENT_DATA];      //  4 Bytes
#endif  
} __PACKED_END TEventLogItem;
//-----------------------------------------------------------------------------
// 报告处理属性
// 事件记录的特征
typedef struct tagEventProperty
{
  uint8_t          Ident;            // 标识 EVTP_xxx宏
  uint8_t          DSFilter;         // 弹窗和保存特性
  uint8_t          NbrOfData;        // 现场数据个数

  uint16_t         OnNameId;         // 0 -> 1 名称
  uint16_t         OffNameId;        // 1 -> 0 名称
  
#if NUM_EVENT_DATA > 0
  // 指定现场数据寄存器
  uint32_t         FieldDataReg[NUM_EVENT_DATA];
#endif
} TEventProperty;
//-----------------------------------------------------------------------------
// 报告描述描述项列表
typedef struct tagEventPropertyList
{
   uint16_t        Count;               // 列表中描述项的数据
  const TEventProperty *List;           // 列表项
} TEventPropertyList;
//-----------------------------------------------------------------------------
// 带属性描述的事件信息
typedef struct tagEventWithProperty
{
         TEventLogItem    EvtLog;
   const TEventProperty  *EvtProp;
   const TDevRegInfoItem *RegInfo;
         char             EvtDesp[16];  // 事件描述
} TEventWithProperty;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 录波结构
#if WAVELOGGER_EN > 0
//-----------------------------------------------------------------------------
// 录波器存储结构
#define NUM_WAVELOG_BINCHLS   (NUM_DigInputs + NUM_Relays)
typedef __PACKED_BEG struct tagWaveLogItem
{
  
  TEventLogSummary  Summary;     // 10 Bytes
  
  uint8_t           Rev[6];      // 保留6字节
  
  // 2 * 40 * 300 * 2 = 48000 Bytes
  uint16_t          Measure[NUM_ADC_CHANNELS][SIZE_SAMPLOG_ADCCHL];     
  // 6 * 300 * 40 / 8 = 9000 Bytes
  uint8_t           Binary [NUM_WAVELOG_BINCHLS][SIZE_WAVELOG_BINCHL];  
} __PACKED_END TWaveLogItem;
#endif // WAVELOGGER_EN
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 事件的环形存贮区结构
typedef __PACKED_BEG struct tagRingStore
{
  uint16_t      ItemLen;                // 单条记录长度
  uint16_t      Total;                  // 最大记录数    (  = NUM_xxxLOGS)
  uint16_t      Count;                  // 已保存的记录数( <= NUM_xxxLOGS)
  uint16_t      Position;               // 当前最新的记录
} __PACKED_END TRingStore;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        日期时间相关 
//-----------------------------------------------------------------------------
// 系统时间
typedef __PACKED_BEG struct tagDateTimeType
{
  uint16_t Year;
  uint8_t  Month;
  uint8_t  Day;
  uint8_t  WeekDay;       // 0~6 0=周日  6=周六
  
  uint8_t  Hours;
  uint8_t  Minutes;
  uint8_t  Seconds;
  uint16_t MilSeconds;
} __PACKED_END TDateTimeType;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        录波相关的数据结构
//-----------------------------------------------------------------------------
// 录波器工作定义
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0
//-----------------------------------------------------------------------------
// 录波轨
typedef struct
{
  uint32_t     RegisterNum;  // 录波轨对应的寄存器, 模拟轨：有效值寄存器 数字轨：实时状态寄存器
  uint16_t     InputChannel; // 录波轨对应的采样通道
  uint16_t     Coefficient;  // 模拟录波轨采样点与实际值的比例关系, 放大100倍
  const char*  Name;         // 录波轨用于显示的名称
} TWaveLogTrackDef;
//-----------------------------------------------------------------------------
// 录波信号源定义
typedef struct
{
  uint8_t            NbrOfADCTracks;   // 有效的模拟录波轨数量
  uint8_t            NbrOfBINTracks;   // 有效有数字录波轨数量

  const TWaveLogTrackDef*  ADCTracks;   // 模拟量录波轨定义数据
  const TWaveLogTrackDef*  BINTracks;   // 数字量录波轨定义数据
} TWaveLogDef;
//-----------------------------------------------------------------------------
// 录波数据区的结构
//-----------------------------------------------------------------------------
// 录波器存储结构
// 12周波： 4056B
// 使用2M Flash时，记录512条
// 要求字节对齐
 #pragma pack(push)
 #pragma pack(1)
typedef __PACKED_BEG struct tagWaveLogItem
{

  TEventLogSummary  Summary;        // 10 Bytes
  uint16_t          uwPerSamples;   // 预采样点数
  uint16_t          uwFAN;          // 故障序号
  uint8_t           Rev[2];         // 保留2字节

  // 12 * 160 * 2 = 3840
  uint16_t          uwMeasure[NUM_WAVELOG_ADCTRACKS][WAVELOG_ADCSAMPS_PER_TRACK];
  // 10 * 160 / 32 * 4 = 200
  uint32_t          uBinary  [NUM_WAVELOG_BINTRACKS][WAVELOG_BINSAMPS_PER_TRACK];
} __PACKED_END TWaveLogItem;
//-----------------------------------------------------------------------------
#pragma pack(pop)
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                        内核数据监视 
//-----------------------------------------------------------------------------
#if AOS_ENABLE > 0
typedef struct tagInspectorAOS
{
  uint8_t     Enable;
  
  uint8_t     RegCount;
  uint32_t    RegNum[NUM_AOS_REG];

  uint8_t     BufRdy;
  uint8_t     BufIdx;
  
  uint16_t    BufAPos;
  uint8_t     BufferA[SIZE_AOS_BUF];
  uint16_t    BufBPos;
  uint8_t     BufferB[SIZE_AOS_BUF];
} TInspectorAOS;
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 计算列表的成员数量
#define NUM_Elements(x)             (sizeof(x) / sizeof(x[0]))
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------

//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
