//-----------------------------------------------------------------------------
/*
  File        : CoilCTRL.cpp
  Version     : V2.10
  By          : 银网科技

  Description : 失压线圈控制器

  Date        : 2023.12.05
  
  v2.1 
   2025.8.10
   适配SG1210v2.1硬件
*/
//-----------------------------------------------------------------------------
#include "CoilCtrl.h"

#include "DevRegs.h"
#include "DevFixed.h"
#include "DevClock.h"
#include "DevEvtMgr.h"

#include "rtc.h"
#include "gpio.h"

#include "BoardCtrl.h"

#include <arm_math.h>
#include <cmsis_os.h>
#include <string.h>
#include <stdio.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 每周波监测次数
#define INSPECTS_PER_PEROID          8

// 电压突变梯度阀值
#define CRITICAL_VOLTAGE_GRADIENT    200000
// 电压突变消抖延时
#define CRITICAL_VOLTAGE_DELAY       (INSPECTS_PER_PEROID / 2)

// 合闸按钮通电时间
#define BREAK_ON_DELAY              60
//-----------------------------------------------------------------------------
// 获取各种数据的宏
// 实时电压
#define ACInput_Voltage             _GetRealReg(REG_RL_UinVal)

// 各种定值
// 门限
#define Threshold_voltage_30        (FController.rVoltageLevel * 30 / 100)
#define Threshold_voltage           (FController.rVoltageLevel * \
                                      _GetDevCfgReg(REG_ACTION_VOLTAGE) / 100)
// 延时
#define Threshold_Delay(reg)        (_GetDevCfgReg(reg) * 60 )  // 分变秒
//-----------------------------------------------------------------------------
// 保存在非易失性存贮器中的状态信息表
#define INDEX_START_DATE            0
#define INDEX_START_TIME            1
#define INDEX_CTRL_STATE            2
#define INDEX_COIL_STATE            3
//-----------------------------------------------------------------------------
#define abs(x)       ((x) >= 0? (x) : -(x))
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
enum TControlerState
{
  csMonitor     = 0,          // 正常监视状态
  csStartup     = 0x01,       // 装置启动
  csPowerLoss   = 0x02,       // 失电保持
  csPowerResume = 0x04,       // 得电闭锁
  csShutDown    = 0x08,       // 等待关机
  csDevFatal    = 0x80        // 装置故障
};
//-----------------------------------------------------------------------------
enum TCoilState
{
  coilPassby    = 0,
  coilStop      = 0x01,
  coilSupport   = 0x02
};
//=============================================================================
// 本地数据申明
//-----------------------------------------------------------------------------
// 控制器状态
typedef struct tagController
{
  // 控制器状态
  TControlerState  stController;
  TCoilState       stCoil1;

  uint32_t         uLastCalcTime;    // 上次计算的时间

  // 用于控制逻辑的状态量
  TDevClock        timeStart;

  TRealReg         rVoltageLevel;
  TRealReg         rVoltage;
  float            fGradient;        // 电压梯度

  uint32_t         uLogicTimer;
  uint32_t         uTickCount;
  uint32_t         uTag;

  float fGradientMax = 0, fGradientMin =0;
} TController;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 控制器状态数据
static TController    FController;

// for debug
#define  NUM_RECDATA       16
#ifdef NUM_RECDATA
 static int iRecPtr = 0, iRecCnt = 0;
 static struct tagRecoder
 {
   TRealReg         rValue;
   float            fGradient;
   uint32_t         uTime;
   uint32_t         uDeltaMicroSec;
 } GRRecorder[NUM_RECDATA];
#endif

//#define  NUM_STEPER       64
#ifdef NUM_STEPER
 static int iStepPtr = 0;
 static struct tagStepRecoder
 {
   uint8_t          ucHour, 
                    ucMin,
                    ucSec;
   uint16_t         uwMilSec;

   TCommonReg       uVoltage;
   float            fGradient;        // 电压梯度

   TControlerState  stCtrl;
   TCoilState       stCoil1;
} Steper[NUM_STEPER];
#endif
//=============================================================================
// 本地方法申明
//-----------------------------------------------------------------------------
static uint32_t _getDuration( const TDevClock* pTimeStart = nullptr );
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 计算控制器算法需要的数据
static void _Calculate()
{

  // 距上次计算的时间
  uint32_t uNow = osKernelGetSysTimerCount();

  uint32_t uDiffSysTick;
  if( uNow >= FController.uLastCalcTime )
    {
    uDiffSysTick = uNow - FController.uLastCalcTime;
    }
  else
    {
    uDiffSysTick = uNow;
    uDiffSysTick += 0xFFFFFFFF - FController.uLastCalcTime + 1;
    }

  // 计算梯度
  auto rVoltage = ACInput_Voltage;
  if( 0 < uDiffSysTick )
    {
    auto rDeltaVoltage = rVoltage - FController.rVoltage;
    uint32_t uFreq = osKernelGetSysTimerFreq();
    float fGradient = rDeltaVoltage * uFreq / uDiffSysTick;  // deltaV/s
    FController.fGradient = fGradient;

    // Test 收集数据
    if( fGradient > 0 &&
        fGradient > FController.fGradientMax )
      FController.fGradientMax = fGradient;
    else if( fGradient < 0 &&
             fGradient < FController.fGradientMin )
      FController.fGradientMin = fGradient;
    }

  FController.rVoltage  = rVoltage;
  FController.uLastCalcTime = uNow;

#ifdef NUM_RECDATA
    // 测试用的代码
  if( iRecPtr > 0 )
    {
    GRRecorder[iRecPtr].rValue    = rVoltage;
    GRRecorder[iRecPtr].fGradient = FController.fGradient;
    GRRecorder[iRecPtr].uTime     = uNow;
    GRRecorder[iRecPtr].uDeltaMicroSec = uDiffSysTick / 168;
    
    if( ++iRecPtr >= NUM_RECDATA )
      {
      iRecPtr = 0;
      }
    }
  else if( abs(FController.fGradient) > CRITICAL_VOLTAGE_GRADIENT )
    {
    GRRecorder[iRecPtr].rValue    = rVoltage;
    GRRecorder[iRecPtr].fGradient = FController.fGradient;
    GRRecorder[iRecPtr].uTime     = uNow;
    GRRecorder[iRecPtr].uDeltaMicroSec = uDiffSysTick / 168;

    iRecPtr = 1;
    iRecCnt++;
    }
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 将工作状态记录到掉电保持寄存器
static void _MarkState()
{

  RTC_WriteCtrlState(INDEX_START_DATE, FController.timeStart.FDays);
  RTC_WriteCtrlState(INDEX_START_TIME, FController.timeStart.FMilSecs);

  RTC_WriteCtrlState(INDEX_CTRL_STATE,  FController.stController);
  RTC_WriteCtrlState(INDEX_COIL_STATE, FController.stCoil1);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 启动线圈供电
static void _InvertorStartup( uint32_t uToken )
{

  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    // 以额定电压启动逆变器
    BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(FController.rVoltageLevel) );
    
    // 启动线圏供电
    BoardCtrl_InvertorOn( TOKEN_BRDSET );
    }
}
//-----------------------------------------------------------------------------
// 启动线圈供电
static void _SwitchCoilSupply( uint32_t uToken )
{

  if( true == TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    auto mode = (TCoilState)TOKEN_BRDSET_GET(uToken);
    switch( mode )
      {
      // 旁路供电模式
      // 1.交流输入：接入
      // 2.旁路继电器：旁路方式
      // 3.逆变器：启动
      case coilPassby:
        {
        // 启动旁路模式
        BoardCtrl_SetPassbyRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_PASSBY, 
                                                        TOKEN_BRDCTRL_ON ) );
        // 逆变器开启
        _InvertorStartup( TOKEN_BRDSET );

        break;
        }

      // 禁止供电模式
      // 1.交流输入：断开
      // 2.旁路继电器：常开结点，合
      // 3.逆变器：关闭
      case coilStop:
        {
        // 逆变器供电
        BoardCtrl_SetPassbyRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_PASSBY, 
                                                        ACout_Passby_OFF ) );
        // 逆变器关闭
        BoardCtrl_InvertorOff( TOKEN_BRDSET );

        break;
        }

      // 逆变供电模式
      // 1.交流输入：断开
      // 2.旁路继电器：常闭结点，开
      // 3.逆变器：启动
      case coilSupport:
        {
        // 逆变器供电
        BoardCtrl_SetPassbyRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_PASSBY, 
                                                        ACout_Passby_OFF ) );
        // 逆变器开启
        _InvertorStartup( TOKEN_BRDSET );

        break;
        }
      }
    }

}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//// 回调线圈供电
//// 线圈启动时，以最高电压使其产生大吸合力，之后回调到常压
//static void _InvertorrVoltageTune()
//{

//  if( 0 == FController.uCoilVolTime && 
//      0 == FController.uCoilVoltageIndex )
//    {
//    FController.uCoilVolTime = 0;
//    return ;
//    }
//  
//  uint32_t uMilSec = FController.uCoilVolTime + 200;
//  if( 24 * 3600000u <= uMilSec )
//    uMilSec -= 24 * 3600000u;
//  if( DevClock.FMilSecs >= uMilSec )
//    {
//    FController.uCoilVoltageIndex--;
//    // 让电压按余弦率下降到设定电压
//    float fRatio = (45 - FController.uCoilVoltageIndex) * 2 * 3.14159f / 180;

//    uint32_t uCoilVoltCfg = _GetDevCfgReg(REG_COIL_VOLTAGE);
//    float fDeltaU = arm_cos_f32(fRatio) * float(FController.uVoltageLevel - uCoilVoltCfg);
//    uint32_t uVoltage = uCoilVoltCfg + fDeltaU;

//    BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(uVoltage), false );
//    
//    if( 0 < FController.uCoilVoltageIndex )
//      FController.uCoilVolTime = DevClock.FMilSecs;
//    else
//      FController.uCoilVolTime = 0;
//    }
//}
//-----------------------------------------------------------------------------
// 线圈保持正常带电
static void _KeepCoilOnDuty()
{

//  if( STATE_FALSE != _GetStateReg( REG_ACPWR_RESUME_LATCH ) )
//    {
//    // 生成事件
//    _SetStateReg      ( REG_ACPWR_RESUME_LATCH, STATE_FALSE );
//    EVTMGR_AppendEvent( REG_ACPWR_RESUME_LATCH, STATE_FALSE );
//    }

//#if ENABLE_PASSBY > 0
//  // 允许旁路？
//  if( 0 == _GetDevCfgReg(REG_PASSBY_EN) )
//#endif
//    {
//    // 启动线圏供电
//    _InvertorStartup( TOKEN_BRDSET );

//    // 启动继电器
////#ifdef REG_COIL1_FUNC
////      if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
////#endif
////      {
////      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
////      FController.stCoil1 = coilSupport;
////      }
////#ifdef REG_COIL1_FUNC
////    else
////      {
////      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
////      FController.stCoil1 = coilStop;
////      }
////#endif

//    SetCoilState(COIL_Normal);
//    }
//#if ENABLE_PASSBY > 0
//  else  // 切换到旁路模式
//    {
//    // 关闭线圏供电
//    BoardCtrl_InvertorOff( TOKEN_BRDSET );

//    // 旁路状态
//    // 调整继电器状态
//    // 常闭结点用于旁路
//    BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
//    FController.stCoil1 = coilPassby;

//    SetCoilState(COIL_Passby);
//    }
//#endif

//  FController.stController = csMonitor;
//  FController.timeStart.FDays  = 0;
//  FController.timeStart.FMilSecs = 0;

//  // 记录状态
//  _MarkState();
}
//-----------------------------------------------------------------------------
// 线圈保持断电
static void _KeepCoilStop(bool bMonitor)
{

//  if( STATE_FALSE != _GetStateReg( REG_ACPWR_LOSS_SUSTAIN ) )
//    {
//    // 生成事件
//    _SetStateReg      ( REG_ACPWR_LOSS_SUSTAIN, STATE_FALSE );
//    EVTMGR_AppendEvent( REG_ACPWR_LOSS_SUSTAIN, STATE_FALSE );
//    }

//  // 关闭线圏驱动电源
//  BoardCtrl_InvertorOff( TOKEN_BRDSET );

//#if ENABLE_PASSBY > 0
//  // 允许旁路？
//  if( 0 == _GetDevCfgReg(REG_PASSBY_EN) )
//#endif
//    {
//    // 关闭继电器，触点在常闭位置
//    BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
//    FController.stCoil1 = coilStop;

//    }
//#if ENABLE_PASSBY > 0
//  else
//    {
//    // 关闭继电器，触点在常开位置，禁止交流从常闭结点向线圈供电
//    BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
//    FController.stCoil1 = coilStop;
//    }
//#endif

//  if( true == bMonitor )
//    {
//    FController.stController = csMonitor;
//    FController.timeStart.FDays  = 0;
//    FController.timeStart.FMilSecs = 0;

//    SetCoilState(COIL_Stop);
//    }
//  
//  // 记录状态
//  _MarkState();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 计算当前动作的持续时间（秒）
static uint32_t _getDuration( const TDevClock* pTimeStart )
{

  uint32_t uDifmilSec = 0;
  
  if( nullptr == pTimeStart )
    pTimeStart = &(FController.timeStart);

  // 计算时间差
  if( pTimeStart->FDays == DevClock.FDays )
    {
    if( pTimeStart->FMilSecs < DevClock.FMilSecs )
      uDifmilSec = DevClock.FMilSecs - pTimeStart->FMilSecs;
    }
  else if( pTimeStart->FDays + 1 == DevClock.FDays )
    {
    uDifmilSec = 24 * 3600000ul - pTimeStart->FMilSecs + DevClock.FMilSecs;
    }

  return uDifmilSec / 1000;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 正常监视
static void _DoMonitor()
{

//  // 梯度超过阀值？
//  if( CRITICAL_VOLTAGE_GRADIENT < abs(FController.fGradient) )
//    {
//    // 电压值在快速变化
//    uint32_t uVoltage    = ACInput_Voltage,
//             uVolLevel   = FController.uVoltageLevel,
//             uVThreshold = Threshold_voltage;

//    FController.uLogicTimer = CRITICAL_VOLTAGE_DELAY;

//    if( 0 < FController.fGradient && uVoltage < uVThreshold )
//      FController.stController = csPowerResume;
//    else if( 0 > FController.fGradient && uVoltage < uVolLevel )
//      {
//      FController.stController = csPowerLoss;
//      }
//    else
//      {
//      FController.uLogicTimer = 0;
//      }
//    }
//  else if( 1000 < abs(FController.fGradient) && 
//           ACInput_Voltage > Threshold_voltage )
//    {
//    // 识别电压等级
//    FController.uLogicTimer++;
//    if( FController.uLogicTimer > 10 )
//      {
//      FController.uLogicTimer = 0;

//      if( ACInput_Voltage > 342 * RATIO_Voltage )
//        {
//        FController.uVoltageLevel = 380 * RATIO_Voltage;
//        }
//      else if( ACInput_Voltage > 198 * RATIO_Voltage )
//        {
//        FController.uVoltageLevel = 220 * RATIO_Voltage;
//        }
//      }
//    }
//  else
//    FController.uLogicTimer = 0;
//  
////  // 线圈在工作状态时，时刻保护电线圈供电和继电器吸合
////  if( coilSupport == FController.stCoil1 && 
////      0x01 != (BoardCtrl_GetCoilsControl() & 0x01) )
////    {
////    // 启动线圏供电
////    _InvertorStartup( TOKEN_BRDSET );

////    // 启动继电器
////#ifdef REG_COIL1_FUNC
////    if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
////#endif
////      {
////      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
////      }
////    }
}
//-----------------------------------------------------------------------------
// 得电过程
static void _DoPowerResumeProc()
{

//  if( FController.uLogicTimer > 0 )
//    {
//    // 消抖延时达到后，启动线圈供电
//    FController.uLogicTimer--;
//    if( 0 == FController.uLogicTimer )
//      {
//      FController.timeStart.FDays  = DevClock.FDays;
//      FController.timeStart.FMilSecs = DevClock.FMilSecs;

//      // 线圈关闭
//      _KeepCoilStop( false );

//      SetCoilState(COIL_KeepOff);

//      // 生成事件
//      _SetStateReg      ( REG_ACPWR_RESUME_LATCH, STATE_TRUE );
//      EVTMGR_AppendEvent( REG_ACPWR_RESUME_LATCH, STATE_TRUE );
//      }
//    else
//      {
//      // 30%定为无压门限
//      uint32_t uVoltage    = ACInput_Voltage,
//               uVol30Latch = Threshold_voltage_30;
//      if( CRITICAL_VOLTAGE_GRADIENT > FController.fGradient &&
//          uVol30Latch > uVoltage )
//        {
//        // 振荡
//        // 延时器复位
//        FController.uLogicTimer = 0;

//        // 关闭线圈供电, 回到监视模式
//        _KeepCoilStop( true );
//        }
//      }
//    }
//  else if( Days2020Since2000 < FController.timeStart.FDays )
//    {
//    // 是否达到闭锁时长
//    uint32_t uTimeCfg = Threshold_Delay(REG_PWRON_TIME);
//    if( _getDuration() > uTimeCfg )
//      {
//      // 切换线圈到持续
//      _KeepCoilOnDuty();
//      }
//    else
//      {
//      // 30%定为无压门限
//      uint32_t uVoltage    = ACInput_Voltage,
//               uVol30Latch = Threshold_voltage_30,
//               uVThreshold = Threshold_voltage;
//      // 电压下降，且低于无压门限
//      if( 0 > FController.fGradient && uVol30Latch > uVoltage )
//        {
//        // 线圈关闭, 回到监视模式
//        _KeepCoilStop( true );
//        }
//      else
//        {
//        // 线圈关闭，继续保持闭锁
//        _KeepCoilStop( false );
//        }
//      }
//    }
//  else if( CRITICAL_VOLTAGE_GRADIENT > FController.fGradient &&
//           ACInput_Voltage >= FController.uVoltageLevel )
//    {
//    // 切换线圈到持续，回到监视模式
//    _KeepCoilOnDuty();
//    }
//  else
//    {
//    // 关闭线圈供电, 回到监视模式
//    _KeepCoilStop( true );
//    }
}
//-----------------------------------------------------------------------------
// 失电过程
static void _DoPowerLossProc()
{

//  if( FController.uLogicTimer > 0 )
//    {
//    // 消抖延时达到后，启动线圈供电
//    FController.uLogicTimer--;
//    if( 0 == FController.uLogicTimer )
//      {
//      // 启动线圏供电
//      _InvertorStartup( TOKEN_BRDSET );

//      FController.timeStart.FDays  = DevClock.FDays;
//      FController.timeStart.FMilSecs = DevClock.FMilSecs;

//      // 记录状态
//      _MarkState();

//      SetCoilState(COIL_KeepOn);

//      // 生成事件
//      _SetStateReg      ( REG_ACPWR_LOSS_SUSTAIN, STATE_TRUE );
//      EVTMGR_AppendEvent( REG_ACPWR_LOSS_SUSTAIN, STATE_TRUE );
//      }
//    else
//      {
//      uint32_t uVoltage  = ACInput_Voltage,
//               uVThreshold = Threshold_voltage;
//      if( -CRITICAL_VOLTAGE_GRADIENT < FController.fGradient && 
//          uVoltage > uVThreshold )
//        {
//        // 延时器复位
//        FController.uLogicTimer = 0;

//        // 切换线圈到持续
//        _KeepCoilOnDuty();
//        }
//      }
//    }
//  else if( Days2020Since2000 < FController.timeStart.FDays )
//    {
//    // 是否达到供电时长
//    uint32_t uTimeCfg = Threshold_Delay(REG_PWROFF_TIME);
//    if( _getDuration() > uTimeCfg )
//      {
//      // 线圈关闭
//      _KeepCoilStop( true );

//      // 准备关机
//      if( 0 != _GetDevCfgReg(REG_AUTO_TURNOFF) )
//        {
//        FController.stController = csShutDown;

//        FController.timeStart.FDays  = DevClock.FDays;
//        FController.timeStart.FMilSecs = DevClock.FMilSecs;

//        // 记录状态
//        _MarkState();

//        SetCoilState(COIL_ShutDownDelay);
//        }
//      }
//    else
//      {
//      uint32_t uVoltage    = ACInput_Voltage,
//               uVThreshold = Threshold_voltage;
//      // 电压回升，且超过有压门限
//      if( 0 < FController.fGradient && uVoltage > uVThreshold )
//        {
//        // 切换线圈到持续, 回到监视模式
//        _KeepCoilOnDuty();
//        }
//      else
//        {
//        // 继续维持线圈供电
//        //------------------------
//        // 启动线圏供电
//        _InvertorStartup( TOKEN_BRDSET );
//        
//        // 启动继电器
////#ifdef REG_COIL1_FUNC
////        if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
////#endif
////          {
////          BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
////          FController.stCoil1 = coilSupport;
////          }
////#ifdef REG_COIL1_FUNC
////        else
////          {
////          BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
////          FController.stCoil1 = coilStop;
////          }
////#endif

//        // 记录状态
//        _MarkState();

//        SetCoilState(COIL_KeepOn);
//        }
//      }
//    }
//  else if( -CRITICAL_VOLTAGE_GRADIENT < FController.fGradient &&
//           ACInput_Voltage < Threshold_voltage_30 )
//    {
//    // 关闭线圈供电, 回到监视模式
//    _KeepCoilStop( true );
//    }
//  else
//    {
//    // 切换线圈到持续, 回到监视模式
//    _KeepCoilOnDuty();
//    }
}
//-----------------------------------------------------------------------------
static void _DoShutDownProc()
{

//  if( Days2020Since2000 < FController.timeStart.FDays )
//    {
//    // 计算时间差
//    uint32_t uDifmilSec = _getDuration();

//    // 是否达到关机时长
//    uint32_t uTimeCfg = Threshold_Delay(REG_SHUTDOWN_TIME);
//    if( uDifmilSec > uTimeCfg )
//      {
//      _KeepCoilStop( true );

//      // 生成事件
//      EVTMGR_AppendEvent( REG_EH_POWEROFF, STATE_TRUE );
//      EVTMGR_SaveEvent();

//      osDelay( 10 );

//      // 关闭装置电源
//      BoardCtrl_Shutdown(TOKEN_BRDSET);

//      SetCoilState(COIL_ShutDown);
//      }
//    else 
//      {
//      // 供电恢复？
//      uint32_t uVoltage    = ACInput_Voltage,
//               uVThreshold = Threshold_voltage;
//      if( uVoltage > uVThreshold && 
//          CRITICAL_VOLTAGE_GRADIENT < FController.fGradient )
//        {
//        // 切换线圈到持续
//        _KeepCoilOnDuty();
//        }
//      }
//    }
//  else
//    {
//    _KeepCoilStop( true );

//    SetCoilState(COIL_Stop);
//    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 启动过程
static void _DoStartupProc()
{

  constexpr auto uSTRATUP_MAX_TRY_COUNT = 2;
  constexpr auto uSTRATUP_POWER_RESUME  = 0xAAAA,
                 uSTRATUP_POWER_LOSS    = 0x3333,
                 uSTRATUP_POWER_STABLE  = 0x5555;

  // 电压值在快速变化
  auto rVoltIn = ACInput_Voltage;

  // 电压变化梯度超过阀值？
  if( CRITICAL_VOLTAGE_GRADIENT < abs(FController.fGradient) )
    {
    if( 0 < FController.fGradient && Threshold_voltage > rVoltIn )
      {
      // 检测到【电压上升】？
      if( uSTRATUP_POWER_RESUME != FController.uTag )
        {
        FController.uTag = uSTRATUP_POWER_RESUME;
          
        FController.uLogicTimer = 1;
        }
      else if( uSTRATUP_MAX_TRY_COUNT < ++FController.uLogicTimer )
        {
        // 在【得电】状态
        FController.stController = csPowerResume;
        }
      }
    else
      {
      // 检测到【电压下落】？
      if( uSTRATUP_POWER_LOSS != FController.uTag )
        {
        FController.uTag = uSTRATUP_POWER_LOSS;
          
        FController.uLogicTimer = 1;
        }
      else if( uSTRATUP_MAX_TRY_COUNT < ++FController.uLogicTimer )
        {
        // 在【失电】状态
        FController.stController = csPowerResume;
        }
      }
    }
  else
    {
    // 电压平稳
    if( uSTRATUP_POWER_STABLE != FController.uTag )
      {
      FController.uTag = uSTRATUP_POWER_STABLE;
        
      FController.uLogicTimer = 1;
      }
    else if( uSTRATUP_MAX_TRY_COUNT < ++FController.uLogicTimer )
      {
      // 延时结束
      // 电压高于阀值
      const auto rCoilVaildVolt  = Threshold_voltage,
                 rInputValidVolt = Threshold_voltage_30;
      if( rCoilVaildVolt < rVoltIn )
        {
        // 在【监控】状态
        FController.stController = csMonitor;

        // 进入【旁路】模式
        _SwitchCoilSupply( TOKEN_BRDSET_SET(coilPassby) );

        // 旁路状态
        SetCoilState( COIL_Passby );
        }
      // 交流输入无压？
      else if( rInputValidVolt > rVoltIn )
        {
        // 起始时刻
        FController.timeStart.FDays    = DevClock.FDays;
        FController.timeStart.FMilSecs = DevClock.FMilSecs;

        // 在【关机】状态
        FController.stController = csShutDown;

        // 进入【停止】模式
        _SwitchCoilSupply( TOKEN_BRDSET_SET(coilStop) );

        // 延时关机
        SetCoilState( COIL_ShutDownDelay );
        }
      }
    }

  // 状态改变？
  if( csStartup != FController.stController )
    {
    FController.uTag        = 0;
    FController.uLogicTimer = 0;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 控制逻辑
static void _Coil_Logic()
{
  
  switch( FController.stController )
    {
    // 正常监视状态  
    case csMonitor:
      {
      _DoMonitor();

      break;
      }
    // 装置启动  
    case csStartup:
      {
      _DoStartupProc();

      break;
      }
    // 失电保持  
    case csPowerLoss:
      {
      _DoPowerLossProc();

      break;
      }
    // 得电闭锁  
    case csPowerResume:
      {
      _DoPowerResumeProc();

      break;
      }
    // 等待关机  
    case csShutDown:
      {
      _DoShutDownProc();

      break;
      }
    // 装置故障  
    case csDevFatal:
      {
      if( 0 == GetHWFault(RHF_LED_MASK) )
        FController.stController = csMonitor;

      break;
      }
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 温度控制
static void _TemperatrueCtrl()
{

  auto rBatTemp = _GetRealReg(REG_RL_BAT_TEMPERATRUE);

  // 开关电源有效
  if( 0 != Adapter_ON_State )
    {
    // 加热器控制
    if( STATE_TRUE == BoardCtrl_GetHeater() )
      {
      if( 5 < rBatTemp )
        {
        auto uTimer = _GetCommonReg( REG_CM_TMR_HEATEROFF );
        if( 0 == uTimer )
          uTimer = 1;
        else
          {
          uTimer++;

          if( 5000 < uTimer )
            {
            // 关闭加热器
            BoardCtrl_HeaterControl( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_HEATER, 
                                                           TOKEN_BRDCTRL_OFF ) );
            uTimer = 0;
            }
          }

        _SetCommonReg(REG_CM_TMR_HEATEROFF, uTimer);
        }
      else
        {
        _SetCommonReg(REG_CM_TMR_HEATEROFF, 0);
        }
      }
    else
      {
      if( -10 > rBatTemp )
        {
        auto uTimer = _GetCommonReg( REG_CM_TMR_HEATERON );
        if( 0 == uTimer )
          uTimer = 1;
        else
          {
          uTimer++;

          if( 5000 < uTimer )
            {
            // 启动加热器
            BoardCtrl_HeaterControl( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_HEATER, 
                                                           TOKEN_BRDCTRL_ON ) );
            uTimer = 0;
            }
          }

        _SetCommonReg(REG_CM_TMR_HEATERON, uTimer);
        }
      else
        {
        _SetCommonReg(REG_CM_TMR_HEATERON, 0);
        }
      }
    }
  else
    {
    _SetCommonReg(REG_CM_TMR_HEATERON,  0);
    _SetCommonReg(REG_CM_TMR_HEATEROFF, 0);
    }

  // 逆变器辅助电源有效时，风扇可用
  if( 0 != Invertor5V_State )
    {
    // 风扇控制
    if( STATE_TRUE == BoardCtrl_GetFanControl() )
      {
      if( 30 > rBatTemp )
        {
        auto uTimer = _GetCommonReg( REG_CM_TMR_FANOFF );
        if( 0 == uTimer )
          uTimer = 1;
        else
          {
          uTimer++;
        
          if( 5000 < uTimer )
            {
            // 关闭加热器
            BoardCtrl_HeaterControl( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_FAN, 
                                                           TOKEN_BRDCTRL_OFF ) );
            uTimer = 0;
            }
          }

        _SetCommonReg(REG_CM_TMR_FANOFF, uTimer);
        }
      else
        {
        _SetCommonReg(REG_CM_TMR_FANOFF, 0);
        }
      }
    else
      {
      if( 45 < rBatTemp )
        {
        auto uTimer = _GetCommonReg( REG_CM_TMR_FANON );
        if( 0 == uTimer )
          uTimer = 1;
        else
          {
          uTimer++;
        
          if( 5000 < uTimer )
            {
            // 启动加热器
            BoardCtrl_HeaterControl( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_FAN, 
                                                           TOKEN_BRDCTRL_ON ) );
            uTimer = 0;
            }
          }

        _SetCommonReg(REG_CM_TMR_FANON, uTimer);
        }
      else
        {
        _SetCommonReg(REG_CM_TMR_FANON, 0);
        }
      }
    }
  else
    {
    _SetCommonReg(REG_CM_TMR_FANON,  0);
    _SetCommonReg(REG_CM_TMR_FANOFF, 0);
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 更新指示灯
static void _UpdateIndictors()
{

  // 得电闭锁指示灯
  if( STATE_FALSE != _GetStateReg( REG_ACPWR_RESUME_LATCH ) )
    // 点状态灯
    SetMLEDState( RMLED_LatchResume );
  else
    // 熄状态灯
    ClrMLEDState( RMLED_LatchResume );

  // 失压保持指示灯
  if( STATE_FALSE != _GetStateReg( REG_ACPWR_LOSS_SUSTAIN ) )
    // 点状态灯
    SetMLEDState( RMLED_KeepLoss );
  else
    // 熄状态灯
    ClrMLEDState( RMLED_KeepLoss );

  // 有压指示灯
  if( _GetRealReg(REG_RL_UinVal) > Threshold_voltage_30 )
    SetMLEDState(RMLED_InvertorOn);
  else
    ClrMLEDState(RMLED_InvertorOn);
  
  // 出口继电器
  if( STATE_TRUE == BoardCtrl_GetRelayState() )
    SetMLEDState(RMLED_CK1On);
  else
    ClrMLEDState(RMLED_CK1On);
  
  // 逆变器
  if( STATE_TRUE == BoardCtrl_GetInvertorPower() )
    SetMLEDState(RMLED_InvertorOn);
  else
    ClrMLEDState(RMLED_InvertorOn);

  // 旁路继电器
  if( STATE_FALSE == BoardCtrl_GetPassby() )
    SetMLEDState(RMLED_Passby);
  else
    ClrMLEDState(RMLED_Passby);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 恢复控制器工作状态
static void _RecoveryState( uint32_t uToken )
{
  
  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    TDevClock  timeStart;
    timeStart.FDays    = RTC_ReadCtrlState(INDEX_START_DATE);
    timeStart.FMilSecs = RTC_ReadCtrlState(INDEX_START_TIME);

    auto uDifmilSec = _getDuration( &timeStart );

    // 根据保存的状态，判定是否还需要继续控制
    auto stCtrl = (TControlerState)RTC_ReadCtrlState(INDEX_CTRL_STATE);
    auto stCoil = (TCoilState)RTC_ReadCtrlState(INDEX_COIL_STATE);
    switch( stCtrl )
      {
      // 失电保持状态
      case csPowerLoss:
        {
        uint32_t uTimeCfg = Threshold_Delay(REG_PWROFF_TIME);
        if( 0 < uDifmilSec && uDifmilSec < uTimeCfg )
          {
          // 线圈供电：逆变器供电方式
          _SwitchCoilSupply( TOKEN_BRDSET_SET(coilSupport) );
            
          // 得电闭锁状态
          SetCoilState( COIL_KeepOff );
          }
        else
          {
          // 启动关机延时
          timeStart.FDays    = DevClock.FDays;
          timeStart.FMilSecs = DevClock.FMilSecs;

          // 线圈供电：停止供电
          _SwitchCoilSupply( TOKEN_BRDSET_SET(coilStop) );

          // 关机延时
          SetCoilState( COIL_ShutDownDelay );
          }

        break;
        }

      // 得电闭锁状态
      case csPowerResume:
        {
        uint32_t uTimeCfg = Threshold_Delay(REG_PWRON_TIME);
        if( 0 < uDifmilSec && uDifmilSec < uTimeCfg )
          {
          // 线圈供电：停止供电
          _SwitchCoilSupply( TOKEN_BRDSET_SET(coilStop) );

          // 得电闭锁状态
          SetCoilState( COIL_KeepOn );
          }
          
        // 还需要送别是否延时结束，但合闸按键延时未结束
        else if( STATE_FALSE != _GetDevCfgReg(REG_AUTO_BREAKER_ON) )
          {
          if( BREAK_ON_DELAY + Threshold_Delay(REG_PWRON_TIME) < uTimeCfg )
            {
            // 线圈供电：旁路供电
            _SwitchCoilSupply( TOKEN_BRDSET_SET(coilPassby) );
              
            // 启动【合闸】继电器
            BoardCtrl_SetRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_CK1, 
                                                      TOKEN_BRDCTRL_ON ) );
            
            // 得电闭锁状态
            SetCoilState( COIL_KeepOn );
            }
          else
            {
            // 线圈供电：旁路供电
            _SwitchCoilSupply( TOKEN_BRDSET_SET(coilPassby) );

            // 旁路状态
            SetCoilState( COIL_Passby );
            }
          }
          
        break;
        }
        
      // 延时关机状态
      case csShutDown:
        {
        // 线圈供电：旁路供电
        _SwitchCoilSupply( TOKEN_BRDSET_SET(coilPassby) );
          
        // 关闭【合闸】继电器
        BoardCtrl_SetRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_CK1, 
                                                  TOKEN_BRDCTRL_OFF ) );

        // 关机延时
        SetCoilState( COIL_ShutDownDelay );

        break;
        }

      // 正常监控状态
      case csMonitor:
        {
        // 根据 stCoil 状态决定线圈的供电方式
        _SwitchCoilSupply( TOKEN_BRDSET_SET(stCoil) );

        // todo

        SetCoilState(COIL_Monitor);

        break;
        }

      default:
        {
        break;
        }
      }

    switch( GetCoilState() )
      {
      case COIL_KeepOn:
        {
        // 在动作过程中复位，继续
        FController.timeStart.FDays    = timeStart.FDays;
        FController.timeStart.FMilSecs = timeStart.FMilSecs;
          
        FController.stController = csPowerResume;
        FController.stCoil1      = coilStop;

        break;
        }
      case COIL_KeepOff:
        {
        // 在动作过程中复位，继续
        FController.timeStart.FDays    = timeStart.FDays;
        FController.timeStart.FMilSecs = timeStart.FMilSecs;
          
        FController.stController = csPowerResume;
        FController.stCoil1      = coilSupport;

        break;
        }
      case COIL_Passby:
        {
        // 在动作过程中复位，继续
        FController.timeStart.FDays    = 0;
        FController.timeStart.FMilSecs = 0;
          
        FController.stController = csMonitor;
        FController.stCoil1      = coilPassby;

        break;
        }
      case COIL_ShutDownDelay:
        {
        // 延时关机过程中复位，继续
        FController.timeStart.FDays    = timeStart.FDays;
        FController.timeStart.FMilSecs = timeStart.FMilSecs;
          
        FController.stController = csShutDown;
        FController.stCoil1      = coilPassby;

        break;
        }
      }

    _MarkState();
    }

}
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
// 控制器定时触发
void CoilCtrl_Tick()
{

//  // 每1/4周期计算一次
//  FController.uTickCount++;
//  if( 0 != (FController.uTickCount % (NUM_SAMPLES_PER_PEROID / INSPECTS_PER_PEROID) ) )
//    return ;

  // 计算被监视数据
  _Calculate();
  
  // 逻辑逻辑
  _Coil_Logic();
  
  // 温度控制
  _TemperatrueCtrl();

  // 更新状态指示
  _UpdateIndictors();
}
//-----------------------------------------------------------------------------
// 启动控制器
// 在控制器任务启动时调用
// 识别开机时刻控制器的状态
void CoilCtrl_Startup(void)
{

  //---------------------------------
#ifdef NUM_STEPER
  memset( Steper, 0, sizeof(Steper) );
#endif

#ifdef NUM_RECDATA
  memset( GRRecorder, 0, sizeof(GRRecorder) );
#endif

  //---------------------------------
  // 初始化控制器状态
  memset( &FController, 0, sizeof(FController) );

  // 控制器状态初值
  FController.stController  = csMonitor;

#ifdef REG_VOLTAGE_LEVEL
  FController.uVoltageLevel = _GetDevCfgReg(REG_VOLTAGE_LEVEL);
  if( 220 * RATIO_Voltage > FController.uVoltageLevel )
#endif
    FController.rVoltageLevel = INVERTER_VOLTAGE;

  // 当前交流电压
  FController.rVoltage  = ACInput_Voltage;

  // 关闭【合闸】继电器
  BoardCtrl_SetRelay( TOKEN_BRDCTRL_ENCODE( TOKEN_BRDCTRL_CK1, 
                                            TOKEN_BRDCTRL_OFF ) );

  // 如果是看门狗动作，恢复工作模式
  if( RRS_IWDGRST == GetRSTSrc(RRS_IWDGRST) )
    {
    // 恢复控制器的工作方式
    _RecoveryState( TOKEN_BRDSET );
    }
  else
    {
    // 非看门狗启动，进入Startup模式，识别状态
    FController.timeStart.FDays    = DevClock.FDays;
    FController.timeStart.FMilSecs = DevClock.FMilSecs;

    FController.stController = csStartup;
      
    FController.uTickCount   = 0;
    FController.uLogicTimer  = 0;

    _SetCommonReg(REG_CM_TMR_STARTUP,  0);

    SetCoilState( COIL_Startup );
    }
}
////-----------------------------------------------------------------------------
//// 控制器实时计算任务
//void CoilCtrl_RTCalc()
//{

//  // 计算被监视数据
//  _Calculate();
//}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
