//-----------------------------------------------------------------------------
/*
 File        : DevRegInfo.h
 Version     : V1.01
 By          : 银网科技

 Description :定义用于描述信息寄存器的属性和处理方法

   卫荣平
   2017.9.12
*/
//-----------------------------------------------------------------------------
#ifndef DEV_REGINFO_H
#define DEV_REGINFO_H

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
// 信号来源
#define SIT_MASK_TYPE     (0x1F << 0)   //
#define SIT_Type_None     (0x00 << 0)   //
#define SIT_Type_Measure  (0x01 << 0)   // 测量数据
#define SIT_Type_Protect  (0x02 << 0)   // 保护数据
#define SIT_Type_Option   (0x03 << 0)   // 设备参数
#define SIT_Type_Config   (0x04 << 0)   // 设备配置
#define SIT_Type_Holding  (0x05 << 0)   // 保护定值
#define SIT_Type_Switch   (0x06 << 0)   // 保护压板
#define SIT_Type_Gear     (0x07 << 0)   // 档位
#define SIT_Type_DigIn    (0x08 << 0)   // 开关量输入
#define SIT_Type_Trip     (0x09 << 0)   // 继电器输出
#define SIT_Type_Fault    (0x0A << 0)   // 保护信号
#define SIT_Type_Event    (0x0B << 0)   // 事件信号
#define SIT_Type_Metering (0x0C << 0)   // 电度脉冲计数
#define STI_Type_Time     (0x0D << 0)   // 日期时间
#define SIT_Type_State    (0x0E << 0)   // 状态数据
#define SIT_GetType(x)    ((x) & SIT_MASK_TYPE)

// 量纲
#define SIT_MASK_DIM      (0x1F << 5)   // 取量纲的宏
#define SIT_DIM_None      (0x00 << 5)   //
#define SIT_DIM_V         (0x01 << 5)   //
#define SIT_DIM_A         (0x02 << 5)   //
#define SIT_DIM_W         (0x03 << 5)   //
#define SIT_DIM_Var       (0x04 << 5)   //
#define SIT_DIM_VA        (0x05 << 5)   //
#define SIT_DIM_Wh        (0x06 << 5)   //
#define SIT_DIM_Varh      (0x07 << 5)   //
#define SIT_DIM_VAh       (0x08 << 5)   //
#define SIT_DIM_Hz        (0x09 << 5)   //
#define SIT_DIM_Deg       (0x0A << 5)   // 角度
#define SIT_DIM_Temp      (0x0B << 5)   // 温度
#define SIT_DIM_Ohm       (0x0C << 5)   // 欧姆
#define SIT_DIM_Perc      (0x0D << 5)   // %
#define SIT_DIM_Min       (0x0E << 5)   // 分
#define SIT_DIM_Sec       (0x0F << 5)   // 秒
#define SIT_DIM_MSec      (0x10 << 5)   // 毫秒
#define SIT_DIM_BPS       (0x11 << 5)   // bit/s
#define SIT_DIM_PT100     (0x12 << 5)   //
#define SIT_DIM_PT220     (0x13 << 5)   //
#define SIT_DIM_PT380     (0x14 << 5)   //
#define SIT_DIM_CT        (0x15 << 5)   //
#define SIT_DIM_4_20TW    (0x16 << 5)   // 4~20mA自动切换温度、湿度

#define SIT_GetDim(x)     ((x) & SIT_MASK_DIM)
#define SIT_GetDimVal(x)  (((x) & SIT_MASK_DIM) >> 5)

// 倍率
#define SIT_MASK_MULTI    (0x03 << 10)
#define SIT_MUL_None      (0x00 << 10)  //
#define SIT_MUL_Kilo      (0x01 << 10)  // K
#define SIT_MUL_Mill      (0x02 << 10)  // 毫
#define SIT_MUL_Mega      (0x03 << 10)  // 兆
#define SIT_GetMulti(x)   ((x) & SIT_MASK_MULTI)

// 倍率切换
#define SIT_MUL_AUTOSHIFT (0x01 << 12)

// 需要乘CT
#define SIT_MULCTPT_SHIFT 13    
#define SIT_MUL_CT        (0x01 << SIT_MULCTPT_SHIFT)  // 13-14
#define SIT_GetMulCT(x)   ((x) & SIT_MUL_CT)
// 需要乘PT
#define SIT_MUL_PT        (0x02 << SIT_MULCTPT_SHIFT)
#define SIT_GetMulPT(x)   ((x) & SIT_MUL_PT)

// 电流定值需要 x 20In
#define SIT_MUL_In_SHIFT  15
#define SIT_MUL_In_MASK   (0x03 << SIT_MUL_In_SHIFT)  // 15-16
#define SIT_MUL_20In      (0x01 << SIT_MUL_In_SHIFT)
#define SIT_MUL_12In      (0x02 << SIT_MUL_In_SHIFT)
#define SIT_GetMulIn(x)   ((x) & SIT_MUL_In_MASK)

// 符号
#define SIT_SIGN_SHIFT    17
#define SIT_UNSIGNED      (0x00 << SIT_SIGN_SHIFT)  // 17
#define SIT_SIGNED        (0x01 << SIT_SIGN_SHIFT)

// 报告分类
// 此类型用于确定报告的存贮和检索分类
// 不指定报告的分类，或类别为 SIT_EVT_None，则事件被忽略
#define SIT_EVT_SHIFT     18
#define SIT_EVT_MASK      (0x07 << SIT_EVT_SHIFT)  // 18-20
#define SIT_EVT_None      (0x00 << SIT_EVT_SHIFT)
#define SIT_EVT_DevLog    (0x01 << SIT_EVT_SHIFT)  // 装置的运行日志
#define SIT_EVT_DevStatus (0x02 << SIT_EVT_SHIFT)  // 装置内部状态，硬件故障，内部部件操作，通信事件
#define SIT_EVT_AutoCtrl  (0x03 << SIT_EVT_SHIFT)  // 装置外部状态引起的事件，输入/输出变化、控制模式变化等
//#define SIT_EVT_Current   (0x04 << SIT_EVT_SHIFT)
//#define SIT_EVT_RmtCtrl   (0x05 << SIT_EVT_SHIFT)
//#define SIT_EVT_Logic     (0x06 << SIT_EVT_SHIFT)
#define SIT_GetEvtType(x) ((x) & SIT_EVT_MASK)

// 报告方式
#define SIT_EVRPT_SHIFT   21                         // 21-22
#define SIT_EVT_Disp      (0x01 << SIT_EVRPT_SHIFT)  // 允许弹出
#define SIT_EVT_Comm      (0x02 << SIT_EVRPT_SHIFT)  // 允许上传

// 事件行为
#define SIT_EVTACT_SHIFT  23                         // 23-24
#define SIT_WAVELOG_TR    (0x01 << SIT_EVTACT_SHIFT) // 录波触发
#define SIT_RELAY_CTRL    (0x02 << SIT_EVTACT_SHIFT) // 继电器

// 显示和编辑类型
#define SIT_VAT_SHIFT     25
#define SIT_VAT_MASK      (0x0F << SIT_VAT_SHIFT)
#define SIT_VAT_INT       (0x00 << SIT_VAT_SHIFT)  // 整数
#define SIT_VAT_HEX       (0x01 << SIT_VAT_SHIFT)  // 16进制
#define SIT_VAT_REAL      (0x02 << SIT_VAT_SHIFT)  // 实数 2026.7.21
#define SIT_VAT_BIN       (0x03 << SIT_VAT_SHIFT)  // 二值状态
#define SIT_VAT_ENUM      (0x04 << SIT_VAT_SHIFT)  // 枚举
#define SIT_VAT_DATETIME  (0x05 << SIT_VAT_SHIFT)  // 日期时间
#define SIT_VAT_PASSWORD  (0x06 << SIT_VAT_SHIFT)  // 密码
#define SIT_VAT_IPADDRv4  (0x07 << SIT_VAT_SHIFT)  // IPv4地址
#define SIT_GetVType(x)   ((x) & SIT_VAT_MASK)
// 访问
#define SIT_RDOnly        (0x80000000)  // 只读
//-----------------------------------------------------------------------------
// 常用综合定义

// 二次电压(保护)
#define SIT_Ux            (SIT_DIM_V | SIT_Type_Protect | SIT_VAT_REAL)
// 一次电压(测量)
#define SIT_MUx           (SIT_DIM_V | SIT_Type_Measure | SIT_VAT_REAL | SIT_MUL_PT)
#define SIT_MKUx          (SIT_MUx   | SIT_MUL_Kilo)

// 二次电流(保护)
#define SIT_Ix            (SIT_DIM_A   | SIT_Type_Protect | SIT_VAT_REAL)
#define SIT_Ix_mA         (SIT_DIM_A   | SIT_MUL_Mill     | SIT_Type_Protect | SIT_VAT_REAL)
#define SIT_Rko           (SIT_DIM_Ohm | SIT_Type_Protect | SIT_VAT_REAL)
// 一次电流(测量)         
#define SIT_MIx           (SIT_DIM_A   | SIT_Type_Measure | SIT_VAT_REAL | SIT_MUL_CT)

#define SIT_Cos           (SIT_DIM_None | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL )
#define SIT_P             (SIT_DIM_W    | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL )
#define SIT_Q             (SIT_DIM_Var  | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL )
#define SIT_S             (SIT_DIM_VA   | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL )

// 角度                   
#define SIT_Phix          (SIT_DIM_Deg | SIT_Type_Measure | SIT_VAT_REAL)

// 测量                   
#define SIT_Cosx          (SIT_DIM_None | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL )

#define SIT_Px            (SIT_DIM_W   | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL)
#define SIT_kPx           (SIT_MUL_Kilo | SIT_Px | SIT_MUL_AUTOSHIFT | SIT_MUL_PT | SIT_MUL_CT)

#define SIT_Qx            (SIT_DIM_Var | SIT_Type_Measure | SIT_SIGNED | SIT_VAT_REAL)
#define SIT_kQx           (SIT_MUL_Kilo | SIT_Qx | SIT_MUL_AUTOSHIFT | SIT_MUL_PT | SIT_MUL_CT)

#define SIT_Sx            (SIT_DIM_VA  | SIT_Type_Measure | SIT_VAT_REAL)
#define SIT_kSx           (SIT_MUL_Kilo | SIT_Sx | SIT_MUL_AUTOSHIFT | SIT_MUL_PT | SIT_MUL_CT)

#define SIT_Freq          (SIT_DIM_Hz | SIT_Type_Measure)

#define SIT_Perc          (SIT_DIM_Perc | SIT_Type_Measure)
#define SIT_Temp          (SIT_DIM_Temp | SIT_Type_Measure)

// 电度
#define SIT_Metering      (SIT_Type_Metering | SIT_MUL_AUTOSHIFT | SIT_VAT_REAL | SIT_UNSIGNED)
#define SIT_Ep            (SIT_MUL_Kilo | SIT_DIM_Wh   | SIT_Metering)
#define SIT_Eq            (SIT_MUL_Kilo | SIT_DIM_Varh | SIT_Metering)
#define SIT_Es            (SIT_MUL_Kilo | SIT_DIM_VAh  | SIT_Metering)

// 装置配置数据
#define SIT_DevOption     ( SIT_Type_Option | SIT_EVT_DevLog )
#define SIT_DevConfig     ( SIT_Type_Config | SIT_EVT_DevLog )

// 软压板
#define SIT_ProSwitch     (SIT_Type_Switch  | SIT_EVT_DevLog  | SIT_EVT_Comm | SIT_VAT_BIN)
// 保护定值
#define SIT_ProHolding    (SIT_Type_Holding | SIT_EVT_DevLog  | SIT_EVT_Comm)

// 硬遥信
#define SIT_DigInput      (SIT_Type_DigIn | SIT_EVT_AutoCtrl  | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_DigInner      (SIT_Type_DigIn | SIT_EVT_DevStatus | SIT_VAT_BIN)

#define SIT_AlmOutput     (SIT_Type_Event)

// 软遥信

//事故软遥信
#define SIT_FaultSign     (SIT_Type_Fault | SIT_EVT_AutoCtrl   | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
//#define SIT_CurrFault     (SIT_Type_Fault | SIT_EVT_Current | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
// 逻辑事件信号
//#define SIT_LogicSign     (SIT_Type_Fault | SIT_EVT_Logic   | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN) 
//告警软遥信
#define SIT_AlarmSign     (SIT_Type_Alarm | SIT_EVT_AutoCtrl | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
//事件软遥信
#define SIT_EventSign     (SIT_Type_Event | SIT_EVT_DevLog   | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)

// 继电器
#define SIT_Relay         (SIT_Type_Trip | SIT_EVT_AutoCtrl  | SIT_EVT_Disp | SIT_EVT_Comm | SIT_RDOnly | SIT_VAT_BIN)
#define SIT_PartCtrl      (SIT_Type_Trip | SIT_EVT_DevStatus | SIT_EVT_Disp | SIT_EVT_Comm | SIT_RDOnly | SIT_VAT_BIN)

// 系统事件
#define SIT_HWE_FAULT      (SIT_Type_Fault | SIT_EVT_DevStatus | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_CFG_FAULT      (SIT_Type_Event | SIT_EVT_DevStatus | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_OPTION_LOG     (SIT_Type_Event | SIT_EVT_DevStatus  | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_OPTION_COM_LOG (SIT_Type_Event | SIT_EVT_DevStatus  | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_RMTCTRL_LOG    (SIT_Type_Even  | SIT_EVT_DevStatus | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
#define SIT_SG_ALL         (SIT_EVT_Comm   | SIT_VAT_BIN)

// 通信事件
#define SIT_COMM_EVENT    (SIT_Type_Event  | SIT_EVT_DevStatus | SIT_EVT_Disp | SIT_EVT_Comm | SIT_VAT_BIN)
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 全局变量
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 获取量纲名称
//-----------------------------------------------------------------------------
// 获取量纲的名称 
// 输入：uRegNum = 寄存器地址
// 返回：0 = 寄存器不存在或寄存器无量纲  != 指向名称字符串的指针
const char* RINF_GetDIMName( uint32_t uRegNum );

const char* RINF_GetDIMNameEx( const TDevRegInfoItem* pProp);
//-----------------------------------------------------------------------------
// 获取量纲的名称 
// 输入：uRegNum = 寄存器地址
// 返回：0 = 寄存器不存在或寄存器无量纲  != 指向名称字符串的指针
const char* RINF_GetDIMName_With_AutoShift( uint32_t uRegNum, uint32_t uValue );
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Get enum option list for a given register
// Returns: item count (0 if not found)
// Output:  pList points to the string ID array
int RINF_getRegEnumList(uint32_t uRegNum, const uint16_t*& pList);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // DEV_REGINFO_H
