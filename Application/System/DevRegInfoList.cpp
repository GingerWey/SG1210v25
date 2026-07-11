//-----------------------------------------------------------------------------
/*
 File        : DevRegInfoList.cpp
 Version     : V1.01
 By          : 银网科技

 Description :定义设备参数寄存器描述列表

   卫荣平
   2017.9.12
*/
//-----------------------------------------------------------------------------
#include "DevRegInfoList.h"

#include "DevRegs.h"
#include "DevRegInfo.h"

#include "Strings\TextStrs.h"

#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define MaxHdBlk      (NUM_HoldingBlocks - 1)
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地常量
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 保护数据
//-----------------------------------------------------------------------------
#define RIP_FREQDISP   0, 0, 45,       55,        1,  1, 5, 2
#define RIP_VOLTDISP   0, 0, MIN_Volt, MAX_Volt,  1,  1, 6, 2
#define RIP_CURRDISP   0, 0, 0,        100,       1,  1, 6, 2
#define RIP_POWRDISP   0, 0, 0,       1000,       1,  1, 6, 1
#define RIP_TEMPDISP   0, 0, -50,     50,         1, 1,  5, 1
#define RIP_PERCDISP   0, 0, 0,       100,        1, 1,  5, 1

static constexpr TDevRegInfoItem pProtDataRegInfos[] =
{
   // Ident  Name    RegNum               MMI       Evt Misc Property
   {  0,     "Fin",  REG_RL_ACFreqRT,    RIP_FREQDISP, 0,  0,    SIT_Freq   }
  ,{  0,     "Fout", REG_RL_VOFreqRT,    RIP_FREQDISP, 0,  0,    SIT_Freq   }
  ,{  0,     "Uin",  REG_RL_UinVal,      RIP_VOLTDISP, 0,  0,    SIT_Ux   }
  ,{  0,     "Uout", REG_RL_UoutVal,     RIP_VOLTDISP, 0,  0,    SIT_Ux   }
};
#define NUM_pProtDataRegInfos         NUM_Elements(pProtDataRegInfos)
//-----------------------------------------------------------------------------
// LVCC保护数据寄存器描述表
const TDevRegInfoList SUTCProtDataRegInfoList =
{
  REG_RL_VOFreqRT,
  REG_RL_UinVal,
  NUM_pProtDataRegInfos,
  pProtDataRegInfos
};
//-----------------------------------------------------------------------------
// 测量数据
//-----------------------------------------------------------------------------
static constexpr TDevRegInfoItem pMeasDataRegInfos[] =
{
   //Ident          Name      RegNum                  MMI          Evt Misc Property
   { idMeasName01,  "Rtemp",  REG_RL_RTC_TEMP,        RIP_TEMPDISP, 0, 0,   SIT_Temp }
  ,{ idMeasName02,  "Rvbat",  REG_RL_RTC_Vbat,        RIP_VOLTDISP, 0, 0,   SIT_Ux   }
  ,{ idMeasName03,  "Fin",    REG_RL_ACFreq,          RIP_FREQDISP, 0, 0,   SIT_Freq }
  ,{ idMeasName04,  "Fout",   REG_RL_VOFreq,          RIP_FREQDISP, 0, 0,   SIT_Freq }
  ,{ idMeasName05,  "Pchrg",  REG_RL_BCHRG_Pbus,      RIP_POWRDISP, 0, 0,   SIT_Px   }
  ,{ idMeasName06,  "Ichrg",  REG_RL_BCHRG_Ibus,      RIP_CURRDISP, 0, 0,   SIT_Ix   }
  ,{ idMeasName07,  "Ichmax", REG_RL_BCHRG_Ibus_Max,  RIP_CURRDISP, 0, 0,   SIT_Ix   }
  ,{ idMeasName08,  "Pdisch", REG_RL_BTOUT_Pbus,      RIP_POWRDISP, 0, 0,   SIT_Px   }
  ,{ idMeasName09,  "Idisch", REG_RL_BTOUT_Ibus,      RIP_CURRDISP, 0, 0,   SIT_Ix   }
  ,{ idMeasName10,  "Idcmax", REG_RL_BTOUT_Ibus_Max,  RIP_CURRDISP, 0, 0,   SIT_Ix   }
  ,{ idMeasName11,  "Blevel", REG_RL_BAT_CAPLevel,    RIP_TEMPDISP, 0, 0,   SIT_Perc }
  ,{ idMeasName12,  "Btemp",  REG_RL_BAT_TEMPERATRUE, RIP_PERCDISP, 0, 0,   SIT_Temp }
  ,{ idMeasName13,  "Uin",    REG_RL_Uin,             RIP_VOLTDISP, 0, 0,   SIT_Ux   }
  ,{ idMeasName14,  "Uout",   REG_RL_Uout,            RIP_VOLTDISP, 0, 0,   SIT_Ux   }
};
#define NUM_pMeasDataRegInfos         NUM_Elements(pMeasDataRegInfos)
//-----------------------------------------------------------------------------
// LVCC测量数据寄存器描述表
const TDevRegInfoList SUTCMeasDataRegInfoList =
{
  REG_RL_RTC_TEMP,
  REG_RL_Uout,
  NUM_pMeasDataRegInfos,
  pMeasDataRegInfos
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 设备配置寄存器 (TDeviceFixed.Options)
//-----------------------------------------------------------------------------
//                RefReg DefValue     Min Max Step Scale Width Dec Evt
#define RIP_OPTION   0,  STATE_FALSE, 0,  1,  1,   1,    1,    0,  0

static constexpr TDevRegInfoItem pDevOptionRegInfos[] =
{
   // Ident          Name  RegNum            RegProp    Misc  Property
   { idDevOptName01, 0,    REG_PROTECT_EN,   RIP_OPTION, 0,  SIT_DevOption | SIT_VAT_BIN }
  ,{ idDevOptName02, 0,    REG_SRLY_AUTORET, RIP_OPTION, 0,  SIT_DevOption | SIT_VAT_BIN }
  ,{ idDevOptName03, 0,    REG_CT_1A,        RIP_OPTION, 0,  SIT_DevOption | SIT_DIM_A | SIT_VAT_ENUM }
  ,{ idDevOptName04, 0,    REG_VOLTAGE_33,   RIP_OPTION, 0,  SIT_DevOption | SIT_VAT_ENUM }
};
#define NUM_DevOpitonRegInfos         (NUM_Elements(pDevOptionRegInfos))
//-----------------------------------------------------------------------------
// 设备配置寄存器描述表
const TDevRegInfoList DevOptionRegInfoList = 
{
  REG_PROTECT_EN,
  REG_VOLTAGE_33,
  NUM_DevOpitonRegInfos,
  pDevOptionRegInfos
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 设备参数寄存器 (TDeviceFixed.Config)
//-----------------------------------------------------------------------------
static constexpr TDevRegInfoItem pDevConfigRegInfos[] =
{
   // Ident          Name   RegNum              RefReg DefValue  Min   Max  Step Scale  Width Dec Evt Misc Property
   { idDevCfgName06,  0,   REG_DEV_ADDR,         0,     0xA5,     1,  0xA5,  0xA5,  1,    1,   0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idDevCfgName07,  0,   REG_AUTOCTRL_EN,      0,     0xA5,     0,  0xA5,  0xA5,  1,    1,   0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idDevCfgName08,  0,   REG_AUTO_TURNOFF,     0,     0xA5,     0,  0xA5,  0xA5,  1,    1,   0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idDevCfgName09,  0,   REG_AUTO_BREAKER_ON,  0,     0xA5,     0,  0xA5,  0xA5,  1,    1,   0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idDevCfgName10,  0,   REG_PASSBY_EN,        0,     0xA5,     0,  0xA5,  0xA5,  1,    1,   0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idDevCfgName11,  0,   REG_ACTION_VOLTAGE,   0,       70,    50,    95,     1,  1,    3,   0,  0,  0,  SIT_DevConfig | SIT_DIM_V    }
  ,{ idDevCfgName12,  0,   REG_COIL_VOLTAGE,     0,      220,   200,   400,    20,  1,    4,   0,  0,  0,  SIT_DevConfig | SIT_DIM_V    }
  ,{ idDevCfgName13,  0,   REG_PWRON_TIME,       0,       10,     1,    60,     1,  1,    5,   0,  0,  0,  SIT_DevConfig | SIT_DIM_Min  }
  ,{ idDevCfgName14,  0,   REG_PWROFF_TIME,      0,       10,     1,    60,     1,  1,    5,   0,  0,  0,  SIT_DevConfig | SIT_DIM_Min  }
  ,{ idDevCfgName15,  0,   REG_SHUTDOWN_TIME,    0,        1,     1,    60,     1,  1,    4,   0,  0,  0,  SIT_DevConfig | SIT_DIM_Sec  }
  ,{ idDevCfgName16,  0,   REG_RELAY_DELAY,      0,        5,     5,   600,     1,  1,    4,   0,  0,  0,  SIT_DevConfig | SIT_DIM_Sec  }
  ,{ idDevCfgName17,  0,   REG_RELAY_TIME,       0,        2,     5,   600,     1,  1,    4,   0,  0,  0,  SIT_DevConfig | SIT_DIM_Sec  }
};
#define NUM_DevConfigRegInfos         (NUM_Elements(pDevConfigRegInfos))
//-----------------------------------------------------------------------------
// 设备参数寄存器描述表
const TDevRegInfoList DevConfigRegInfoList = 
{
  REG_DEV_ADDR,
  REG_RELAY_TIME,
  NUM_DevConfigRegInfos,
  pDevConfigRegInfos
};
//=============================================================================
// 状态寄存器
//-----------------------------------------------------------------------------
//                  RefReg DefValue Min Max Step Scale Width Dec
#define RIP_SIGNAL    0,      0,    0,   0,  1,   1,    1,    0  
//-----------------------------------------------------------------------------
//static constexpr TDevRegInfoItem  pSysStateRegInfos[] =
//{
//   // Ident        Name RegNum                   ....         Evt           Misc Property
//   // GOOSE事件
//   { idETHNETBreak,  0, REG_EthNet_Break,        RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSCOMMBreak,  0, REG_GOOSE_Break,         RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName01,  0, REG_GOOSE_BreakOff(0),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName02,  0, REG_GOOSE_BreakOff(1),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName03,  0, REG_GOOSE_BreakOff(2),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName04,  0, REG_GOOSE_BreakOff(3),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName05,  0, REG_GOOSE_BreakOff(4),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName06,  0, REG_GOOSE_BreakOff(5),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName07,  0, REG_GOOSE_BreakOff(6),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName08,  0, REG_GOOSE_BreakOff(7),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName09,  0, REG_GOOSE_BreakOff(8),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName10,  0, REG_GOOSE_BreakOff(9),   RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName11,  0, REG_GOOSE_BreakOff(10),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName12,  0, REG_GOOSE_BreakOff(11),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName13,  0, REG_GOOSE_BreakOff(12),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName14,  0, REG_GOOSE_BreakOff(13),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName15,  0, REG_GOOSE_BreakOff(14),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//  ,{ idGSEvtName16,  0, REG_GOOSE_BreakOff(15),  RIP_SIGNAL,  EVTP_GOOSEEVT, 0, SIT_COMM_EVENT }
//};
//#define NUM_SysStateRegInfos          (NUM_Elements(pSysStateRegInfos))
////-----------------------------------------------------------------------------
//// 系统状态寄存器描述表
//const TDevRegInfoList SysStateRegInfoList = 
//{
//  REG_DEVADDR,
//  REG_GOOSE_BreakOff(15),
//  NUM_SysStateRegInfos,
//  pSysStateRegInfos
//};
//=============================================================================
// 事件寄存器
//-----------------------------------------------------------------------------
//                 RefReg DefValue Min Max   Step Scale Width Dec
static constexpr TDevRegInfoItem   pEventLogRegInfos[] =
{
   // Ident         Name   RegNum                    Reg_Disp        Evt       Misc  Property
   { idEventName00,   0,   REG_EH_POWEROFF,          RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName01,   0,   REG_EH_POWERON,           RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName02,   0,   REG_EH_POWERON_PWR,       RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName03,   0,   REG_EH_POWERON_WDG,       RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName04,   0,   REG_EH_POWERON_SFT,       RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName08,   0,   REG_EH_WDG_ACTIVE,        RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_EVT_DevLog | SIT_VAT_BIN }
  ,{ idEventName09,   0,   REG_EH_HSE_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName10,   0,   REG_EH_LSE_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName11,   0,   REG_EH_RCC_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName12,   0,   REG_EH_BAT_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName13,   0,   REG_EH_RTC_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName14,   0,   REG_EH_FeRAM_FAULT,       RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName15,   0,   REG_EH_ExFLASH_FAULT,     RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName16,   0,   REG_EH_ADC_FAULT,         RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName17,   0,   REG_EH_EXADC1_FAULT,      RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName18,   0,   REG_EH_EXADC2_FAULT,      RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName19,   0,   REG_EH_ExRTC_FAULT,       RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName20,   0,   REG_EH_BATLOW_FAULT,      RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }
  ,{ idEventName21,   0,   RHF_EH_RHEOSTAT_FAULT,    RIP_SIGNAL,  EVTP_HWFAULT, 0,  SIT_HWE_FAULT }

  // 配置数据相关事件
  ,{ idEventName30,   0,   REG_EC_DEVCFGERR,         RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName31,   0,   REG_EC_DEVCFGINIT,        RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName32,   0,   REG_EC_HOLDINGERR,        RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName33,   0,   REG_EC_HOLDINGINIT,       RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName34,   0,   REG_EC_EVENTLOGERR,       RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName35,   0,   REG_EC_EVENTLOGINIT,      RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName36,   0,   REG_EC_HOLDINGFAULT,      RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName37,   0,   REG_EC_HOLDINGRECOVER,    RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
#if METERING_EN > 0
  ,{ idEventName40   0,   REG_EC_METERINGFAULT,     RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName41,  0,   REG_EC_METERINGINIT,      RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
  ,{ idEventName42,  0,   REG_EC_METERINGOVERFLOW,  RIP_SIGNAL,  EVTP_CFGFAULT, 0,  SIT_CFG_FAULT }
#endif
  // 装置操作事件
  ,{ idEventName50,   0,   REG_EO_SET_RTC,           RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName51,   0,   REG_EO_CLR_EVENT,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName52,   0,   REG_EO_CLR_ALARM,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName53,   0,   REG_EO_CLR_FAULT,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName54,   0,   REG_EO_CLR_WAVELOG,       RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
//  ,{ idEventName55,   0,   REG_EO_RST_METERING,      RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName56,   0,   REG_EO_DEF_HOLDING,       RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName57,   0,   REG_EO_DEF_DEVCFG,        RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName58,   0,   REG_EO_CHGACTHOLD,        RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName59,   0,   REG_EO_COPYHOLDING,       RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName60,   0,   REG_EO_SETDEVCFG,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName61,   0,   REG_EO_SETHLDSWT,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName62,   0,   REG_EO_SETHLDATA,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName63,   0,   REG_EO_SETNETCFG,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName64,   0,   REG_EO_SETCTRLMX,         RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName65,   0,   REG_EO_SETPASSWORD,       RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName66,   0,   REG_EO_SETPASSWORDx,      RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
  ,{ idEventName67,   0,   REG_EO_SETCALIBRATION,    RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
//  ,{ idEventName68,   0,   REG_EO_SETDEVFUNCTYPE,    RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
//  ,{ idEventName69,   0,   REG_EO_SETDISTRIBUTOR,    RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }
//  ,{ idEventName70,   0,   REG_EO_SIGNALS_RETURN,    RIP_SIGNAL,  EVTP_DIGINPT,  0,  SIT_OPTION_COM_LOG}
//  ,{ idEventName71,   0,   REG_EO_WAVELOG_TRIGGER,   RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_OPTION_LOG }

//  ,{ idEventName80,   0,   REG_EO_BREAK_ON_OPEATE,   RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_OPTION_LOG }
//  ,{ idEventName81,   0,   REG_EO_BREAK_OFF_OPEATE,  RIP_SIGNAL,  EVTP_OPERATE, 0,  SIT_OPTION_LOG }

#ifdef SIT_EVT_RmtCtrl
  // 遥控操作事件
  ,{ idEventName60,   0,   REG_EM_SET_RTC,           RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_RMTCTRL_LOG }
  ,{ idEventName61,   0,   REG_EM_CONFIG,            RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_RMTCTRL_LOG }
  ,{ idEventName62,   0,   REG_ER_SGLRET,            RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_RMTCTRL_LOG }
                                                     
  // 遥控事件                                        
  ,{ idEventName70,   0,   REG_ER_RELAY0,            RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_RMTCTRL_LOG }
  ,{ idEventName71,   0,   REG_ER_RELAY1,            RIP_SIGNAL,  EVTP_OPERATE,  0,  SIT_RMTCTRL_LOG }
#endif
};
#define NUM_EventLogRegInfos          (NUM_Elements(pEventLogRegInfos))
//-----------------------------------------------------------------------------
// 设备参数寄存器描述表
const TDevRegInfoList EventReportRegInfoList =
{
  REG_EH_POWEROFF,
  REG_EO_SETCALIBRATION,
  NUM_EventLogRegInfos,
  pEventLogRegInfos
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 功能寄存器
//-----------------------------------------------------------------------------
static constexpr TDevRegInfoItem pDevFunctionRegInfos[] =
{  // 只有功能寄存RefReg配置为REG_DEVOPTION/REG_DEVCONFIG，才会被填写默认参数。
   // Ident            Name   RegNum             RefReg        DefValue Min Max  Step Scale Width Dec Evt Misc Property
   { idDevOptName01,     0,  REG_FN_PROTECT_EN,    0,               0,   0,  1,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_BIN  }
//  ,{ idDevOptName02,     0,  REG_FN_SOE_AUTORET,   0,               0,   0,  1,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_BIN  }
//  ,{ idDevOptName03,     0,  REG_FN_CT_1A,         0,               0,   0,  1,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
//  ,{ idDevOptName04,     0,  REG_FN_VOLTAGE_MODE,  0,               0,   0,  1,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }

//  ,{ idDateTimeName01,   0,  REG_FN_DATETIME,       0,              0,    0,  0,    0,   0,    4,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName02,   0,  REG_DATE_YEAR,         0,           2025,  2010, 2100, 1,   1,    4,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName03,   0,  REG_DATE_MONTH,        0,              1,    1,  12,   1,   1,    2,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName04,   0,  REG_DATE_DAY,          0,              1,    1,  31,   1,   1,    2,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName06,   0,  REG_TIME_HOUR,         0,              0,    0,  23,   1,   1,    2,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName07,   0,  REG_TIME_MINUTE,       0,              0,    0,  59,   1,   1,    2,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName08,   0,  REG_TIME_SECOND,       0,              0,    0,  59,   1,   1,    2,    0,  0,  0,  STI_Type_Time }
//  ,{ idDateTimeName09,   0,  REG_TIME_MSECOND,      0,              0,    0, 999,   1,   1,    3,    0,  0,  0,  STI_Type_Time }

  // 串口配置
  ,{ idUARTCfgName01,    0,  REG_UART1_CFG,         0,              0,    0,  0,    1,   1,    3,    0,  0,  0,  SIT_DevConfig | SIT_VAT_UART }
  ,{ idUARTCfgName02,    0,  REG_UART1_ADDR,        REG_DEVCONFIG,  252,  1,  247,  1,   1,    3,    0,  0,  0,  SIT_DevConfig }
  ,{ idUARTCfgName03,    0,  REG_UART1_BAUDRATE,    REG_DEVCONFIG,  3,    0,  7,    1,   1,    5,    0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
  ,{ idUARTCfgName04,    0,  REG_UART1_PARITY,      REG_DEVCONFIG,  0,    0,  2,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
//  ,{ idUARTCfgName05,    0, REG_UART1_STOPBITS,  REG_DEVCONFIG,     0,    0,  1,    1,   1,    1,    0,  0,  0,  SIT_DevConfig | SIT_VAT_ENUM }
};                                                            
#define NUM_DevFunctionRegInfos       (NUM_Elements(pDevFunctionRegInfos))
//-----------------------------------------------------------------------------
// 设备级功能寄存器描述表                                     
const TDevRegInfoList DevFunctionRegInfoList =                
{                                                             
  REG_FN_PROTECT_EN,                                          
  REG_UART1_PROTOCOL,                                       
  NUM_DevFunctionRegInfos,                                    
  pDevFunctionRegInfos
};                                                            
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 配置寄存器 开入别名                                        
//-----------------------------------------------------------------------------
////                Min    Max       Step Scale Width Dec Evt Misc
//#define RIP_ALIAS  0,    (42 - 1),  1,   1,    1,   0,  0,  0 

//static constexpr TDevRegInfoItem pDevCfgDIAliasRegInfos[] =
//{                                                             
//   // Ident       Name  RegNum       RefReg DefValue RegProp     Property
//   { idDIAlias00,  0,    REG_DIAlais1,  0,     1,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias01,  0,    REG_DIAlais2,  0,     2,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias02,  0,    REG_DIAlais3,  0,     3,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias03,  0,    REG_DIAlais4,  0,     4,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias04,  0,    REG_DIAlais5,  0,     5,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias05,  0,    REG_DIAlais6,  0,     6,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias06,  0,    REG_DIAlais7,  0,     7,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias07,  0,    REG_DIAlais8,  0,     8,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias08,  0,    REG_DIAlais9,  0,     9,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//  ,{ idDIAlias09,  0,    REG_DIAlais10, 0,    10,     RIP_ALIAS,  SIT_DevConfig | SIT_VAT_ENUM  }
//};
//#define NUM_pDevCfgDIAliasRegInfos    (NUM_Elements(pDevCfgDIAliasRegInfos))
////-----------------------------------------------------------------------------
//// 开入别名寄存器描述表
//const TDevRegInfoList DevCfgDIAliasRegInfoList = 
//{
//  REG_DIAlais1,
//  REG_DIAlais10,
//  NUM_pDevCfgDIAliasRegInfos,
//  pDevCfgDIAliasRegInfos
//};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 开入状态寄存器.硬遥信
//-----------------------------------------------------------------------------
//                    DefValue Min Max Step Scale Width Dec Evt
#define RIP_DIREGPROP    0,    0,  1,  1,   1,    1,    0,  EVTP_DIGINPT

static constexpr TDevRegInfoItem  pStateDigitaInputRegInfos[] =
{
   // Ident       Name RegNum    RefReg  DISP            Misc Property
   { idDIName00,  0,   REG_DI0,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName01,  0,   REG_DI1,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName02,  0,   REG_DI2,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName03,  0,   REG_DI3,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName04,  0,   REG_DI4,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName05,  0,   REG_DI5,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName06,  0,   REG_DI6,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
  ,{ idDIName07,  0,   REG_DI7,  0,      RIP_DIREGPROP,  0,   SIT_DigInput }
};
#define NUM_StateDigitaInputRegInfos  (NUM_Elements(pStateDigitaInputRegInfos))
//-----------------------------------------------------------------------------
const TDevRegInfoList StateDigitaInputRegInfoList =
{
  REG_DI0,
  REG_DI7,
  NUM_StateDigitaInputRegInfos,
  pStateDigitaInputRegInfos
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 开入功能寄存器. 硬遥信，状态从Alias映射
//-----------------------------------------------------------------------------
#ifdef CNT_DigInAlais
static constexpr TDevRegInfoItem pStateDIFuncRegInfos[] =
{
   // Ident         Name   RegNum       RefReg DISP          Misc  Property
   { idDIAliasName00, 0,   REG_DI_FUNC00, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName01, 0,   REG_DI_FUNC01, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName02, 0,   REG_DI_FUNC02, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName03, 0,   REG_DI_FUNC03, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName04, 0,   REG_DI_FUNC04, 0,   RIP_DIREGPROP,  0,  SIT_DigInput1 }
  ,{ idDIAliasName05, 0,   REG_DI_FUNC05, 0,   RIP_DIREGPROP,  0,  SIT_DigInput1 }
  ,{ idDIAliasName06, 0,   REG_DI_FUNC06, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName07, 0,   REG_DI_FUNC07, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName08, 0,   REG_DI_FUNC08, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName09, 0,   REG_DI_FUNC09, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
  ,{ idDIAliasName10, 0,   REG_DI_FUNC10, 0,   RIP_DIREGPROP,  0,  SIT_DigInput }
};
#define NUM_StateDIFuncRegInfos       (NUM_Elements(pStateDIFuncRegInfos))
//-----------------------------------------------------------------------------
// 开入功能状态寄存器描述表
const TDevRegInfoList StateDIFuncRegInfoList = 
{
  REG_DI_FUNC00,
  REG_DI_FUNC59,
  NUM_StateDIFuncRegInfos,
  pStateDIFuncRegInfos
};
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 开出状态寄存器
//-----------------------------------------------------------------------------
//                   DefValue Min Max Step Scale Width Dec Evt 
#define RIP_DOREGPROP   0,    0,  1,  1,   1,    1,    0,  EVTP_RELAY
#define RIP_PERIPHPROP  0,    0,  1,  1,   1,    1,    0,  EVTP_PERIPH

static constexpr TDevRegInfoItem pStateRelayOutputRegInfos[] =
{
   // Ident         Name RegNum     RefReg RegProp        Misc  Property
   { idRlyState00,  0,   REG_RELAY0,  0,   RIP_DOREGPROP,  0,  SIT_Relay }
  ,{ idRlyState01,  0,   REG_RELAY1,  0,   RIP_DOREGPROP,  0,  SIT_Relay }
  ,{ idRlyState02,  0,   REG_RELAY2,  0,   RIP_PERIPHPROP, 0,  SIT_Relay }
  ,{ idRlyState03,  0,   REG_RELAY3,  0,   RIP_PERIPHPROP, 0,  SIT_Relay }
};
#define NUM_pStateRelayOutputRegInfos (NUM_Elements(pStateRelayOutputRegInfos))
//-----------------------------------------------------------------------------
// 开出状态寄存器描述表
const TDevRegInfoList StateRelayOutputRegInfoList = 
{
  REG_RELAY0,
  REG_RELAY3,
  NUM_pStateRelayOutputRegInfos,
  pStateRelayOutputRegInfos
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 通用寄存器.直流测量结果
//-----------------------------------------------------------------------------
////                   DefValue Min Max Step Scale     Width Dec Evt 
//#define RIP_DCREGPROP   0,    0,  1,  1,   RATIO_DC, 6,    2,  0
////
//constexpr TDevRegInfoItem pDCInputMeasureRegInfos[] =
//{
//   // Ident         Name RegNum     RefReg RegProp       Misc Property
//   { idDCInpName01, 0,   REG_DC0Res, 0,    RIP_DCREGPROP, 0,  SIT_DIM_V | SIT_SIGNED }
//  ,{ idDCInpName02, 0,   REG_DC1Res, 0,    RIP_DCREGPROP, 0,  SIT_DIM_V | SIT_SIGNED }
//};
//#define NUM_pDCInputMeasureRegInfos   NUM_Elements(pDCInputMeasureRegInfos))
////-----------------------------------------------------------------------------
//// 直流测量结果寄存器描述表
//static constexpr TDevRegInfoList DCInputMeasureRegInfos = 
//{
//  REG_DC0Res,
//  REG_DC1Res,
//  NUM_pDCInputMeasureRegInfos,
//  pDCInputMeasureRegInfos
//};
////-----------------------------------------------------------------------------
////-----------------------------------------------------------------------------
//// 传动寄存器.开入传动
////-----------------------------------------------------------------------------
////                       DefValue Min Max Step Scale Width Dec Evt
//#define RIP_DITSTREGPROP    0,    1,  0,  1,   1,    1,    0,  0

//static constexpr TDevRegInfoItem pDITesttRegInfos[] =
//{
//   // Ident          Name  RegNum           RefReg      DISP             Misc Property
//   { idDITestName00,  0,   REG_DITEST + 0,  REG_DI0,    RIP_DITSTREGPROP, 0, SIT_DigInput }
//  ,{ idDITestName01,  0,   REG_DITEST + 1,  REG_DI1,    RIP_DITSTREGPROP, 0, SIT_DigInput }
//  ,{ idDITestName02,  0,   REG_DITEST + 2,  REG_DI2,    RIP_DITSTREGPROP, 0, SIT_DigInput }
//  ,{ idDITestName03,  0,   REG_DITEST + 3,  REG_DI3,    RIP_DITSTREGPROP, 0, SIT_DigInput }
//  ,{ idDITestName04,  0,   REG_DITEST + 4,  REG_DI4,    RIP_DITSTREGPROP, 0, SIT_DigInput }
//};
//#define NUM_pDITesttRegInfos          (NUM_Elements(pDITesttRegInfos))
////-----------------------------------------------------------------------------
//// 开入传动寄存器描述表
//const TDevRegInfoList DigInputTestRegInfos =
//{
//  REG_DITEST + 0,
//  REG_DITEST + NUM_pDITesttRegInfos - 1,
//  NUM_pDITesttRegInfos,
//  pDITesttRegInfos
//};
////-----------------------------------------------------------------------------
//// 开出传动寄存器
////-----------------------------------------------------------------------------
////                       DefValue Min Max Step Scale Width Dec Evt 
//#define RIP_DOTESTREGPROP   0,    0,  1,  1,   1,    1,    0,  EVTP_RELAY

//static constexpr TDevRegInfoItem pRelayOutputTestRegInfos[] =
//{
//   // Ident       Name RegNum           RefReg       RegProp           Misc  Property
//   { idRlyTest00,  0,   REG_DOTEST + 0,  REG_RELAY0,  RIP_DOTESTREGPROP, 0,  SIT_Relay }
//  ,{ idRlyTest01,  0,   REG_DOTEST + 1,  REG_RELAY1,  RIP_DOTESTREGPROP, 0,  SIT_Relay }
//};
//#define NUM_pRelayOutpuTestRegInfos   (NUM_Elements(pRelayOutputTestRegInfos))
////-----------------------------------------------------------------------------
//// 开出传动寄存器描述表
//constexpr TDevRegInfoList RelayOutputTestRegInfoList = 
//{
//  REG_DOTEST + 0,
//  REG_DOTEST + NUM_pRelayOutpuTestRegInfos - 1,
//  NUM_pRelayOutpuTestRegInfos,
//  pRelayOutputTestRegInfos
//};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 配置各种事件类型的特性
static constexpr TEventProperty pEventPropertyItems[] =
{
  // Ident           DSFilter               NbrOfData  OnName     OffName      FieldDataReg
   { EVTP_OPERATE,   EDS_SAVEBOTH,              0,  idEventAct00, idEventAct00, {0} }
  ,{ EVTP_CFGFAULT,  EDS_DISP01 | EDS_SAVE01,   0,  idEventAct00, idEventAct00, {0} }
  ,{ EVTP_HWFAULT,   EDS_DISP01 | EDS_SAVE01,   0,  idEventAct00, idEventAct00, {0} }
//  ,{ EVTP_PROSWTH,   EDS_DISP01 | EDS_SAVE01  0,  idEventAct00, idEventAct00, {0} }
  ,{ EVTP_DIGINPT,   EDS_DISP01 | EDS_SAVEBOTH, 0,  idEventAct11, idEventAct12, {0} }
  ,{ EVTP_RELAY,     EDS_DISP01 | EDS_SAVE01,   0,  idEventAct11, idEventAct12, {0} }
  ,{ EVTP_PERIPH,    EDS_DISP01 | EDS_SAVE01,   0,  idEventAct31, idEventAct32, {0} }
  ,{ EVTP_PROTECT1,  EDS_DISP01 | EDS_SAVEBOTH, 1,  idEventAct21, idEventAct22, {REG_RL_UinVal} }
};
#define NUM_EventPropertyItems        (NUM_Elements(pEventPropertyItems))
//-----------------------------------------------------------------------------
// LVCC的事件报告属性
const TEventPropertyList SUTCEventPropertys =
{
   NUM_EventPropertyItems,
   pEventPropertyItems
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 软遥信
//-----------------------------------------------------------------------------
static constexpr TDevRegInfoItem plistProtSignRegInfos[] =
{
   //  Ident     Name  RegNum                   MMI          EventProperty  Misc Property
   { idSGName01, 0,    REG_ACPWR_RESUME_LATCH,  RIP_SIGNAL,  EVTP_PROTECT1, 0,   SIT_FaultSign | SIT_RELAY_CTRL | SIT_WAVELOG_TR } //  0
  ,{ idSGName02, 0,    REG_ACPWR_LOSS_SUSTAIN,  RIP_SIGNAL,  EVTP_PROTECT1, 0,   SIT_FaultSign | SIT_RELAY_CTRL | SIT_WAVELOG_TR } //  0
  ,{ idSGName03, 0,    REG_AUTO_CLOSE_BREAKER,  RIP_SIGNAL,  EVTP_PROTECT1, 0,   SIT_FaultSign | SIT_RELAY_CTRL | SIT_WAVELOG_TR } //  0
};
#define NUM_pProtSignRegInfos       NUM_Elements(plistProtSignRegInfos)
//-----------------------------------------------------------------------------
// 软遥信寄存器描述表
const TDevRegInfoList ProtSignRegInfoList =
{
  REG_ACPWR_RESUME_LATCH,
  REG_ACPWR_LOSS_SUSTAIN,
  NUM_pProtSignRegInfos,
  plistProtSignRegInfos
};
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
