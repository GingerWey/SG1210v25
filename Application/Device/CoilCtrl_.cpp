//-----------------------------------------------------------------------------
/*
  File        : CoilCTRL.cpp
  Version     : V1.10
  By          : 银网科技

  Description : 失压线圈控制器

  Date        : 2023.12.05
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
//-----------------------------------------------------------------------------
// 获取各种数据的宏
// 实时电压
#define RealTime_Voltage             _GetCommonReg(REG_CM_UinVal)

// 各种定值
#define Threshold_voltage_30         (FController.uVoltageLevel * 30 / 100)
#define Threshold_voltage            (FController.uVoltageLevel * \
                                      _GetDevCfgReg(REG_ACTION_VOLTAGE) / 100)
#define Threshold_Delay(reg)         (_GetDevCfgReg(reg) *       \
                                      (1000 / RATIO_Delay_Second))
//-----------------------------------------------------------------------------
// 保存在非易失性存贮器中的状态信息表
#define INDEX_START_DATE      0
#define INDEX_START_TIME      1
#define INDEX_CTRL_STATE      2
#define INDEX_COIL1_STATE     3
#define INDEX_COIL2_STATE     4
//-----------------------------------------------------------------------------
#define abs(x)       ((x) >= 0? (x) : -(x))
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
enum TControlerState
{
  csMonitor     = 0,
  csPowerLoss   = 0x01,
  csPowerResume = 0x02,
  csShutDown    = 0x04
};
//-----------------------------------------------------------------------------
enum TCoilState
{
  coilPassby    = 0,
  coilStop      = 0x01,
  coilOnDuty    = 0x02
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

  TCommonReg       uVoltageLevel;
  TCommonReg       uVoltage;
  float            fGradient;        // 电压梯度

  uint32_t         uLogicTimer;
  uint32_t         uTickCount;

  float fGradientMax = 0, fGradientMin =0;
  
  // 用于调整线圈电压的状态量
  uint32_t         uCoilVoltageIndex;
  uint32_t         uCoilVolTime;
  
} TController;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 控制器状态数据
static TController    FController;

// for debug
//#define  NUM_RECDATA       16
#ifdef NUM_RECDATA
 static int iRecPtr = 0, iRecCnt = 0;
 static struct tagRecoder
 {
   TCommonReg       uValue;
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
  TCommonReg uVoltage = RealTime_Voltage;
  if( uDiffSysTick > 0 )
    {
    int iDeltaVoltage = (int)uVoltage - (int)(FController.uVoltage);
    uint32_t uFreq = osKernelGetSysTimerFreq();
    float fGradient = (float)iDeltaVoltage * uFreq / uDiffSysTick;  // deltaV/s
    FController.fGradient = fGradient;

    // Test 收集数据
    if( fGradient > 0 &&
        fGradient > FController.fGradientMax )
      FController.fGradientMax = fGradient;
    else if( fGradient < 0 &&
             fGradient < FController.fGradientMin )
      FController.fGradientMin = fGradient;
    }

  FController.uVoltage  = uVoltage;
  FController.uLastCalcTime = uNow;

#ifdef NUM_RECDATA
    // 测试用的代码
  if( iRecPtr > 0 )
    {
    GRRecorder[iRecPtr].uValue    = uVoltage;
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
    GRRecorder[iRecPtr].uValue    = uVoltage;
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
  RTC_WriteCtrlState(INDEX_COIL1_STATE, FController.stCoil1);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 启动线圈供电
static void _InvertorStartup( uint32_t uToken )
{

  if( TOKEN_BRDSET_ISCORRECT(uToken) && 0 == BoardCtrl_GetInvertorPower() )
    {
    // 以最高电压启动, 产生最大吸力
    BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(INVERTORVOL_MAX), true );
    
    // 启动线圏供电
    BoardCtrl_InvertorOn( TOKEN_BRDSET );

    // 
    FController.uCoilVoltageIndex = 45;
    FController.uCoilVolTime      = DevClock.FMilSecs + 1000;
    if( 24 * 3600000u <= FController.uCoilVolTime )
      FController.uCoilVolTime -= 24 * 3600000u;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 回调线圈供电
// 线圈启动时，以最高电压使其产生大吸合力，之后回调到常压
static void _InvertorrVoltageTune()
{

  if( 0 == FController.uCoilVolTime && 
      0 == FController.uCoilVoltageIndex )
    {
    FController.uCoilVolTime = 0;
    return ;
    }
  
  uint32_t uMilSec = FController.uCoilVolTime + 200;
  if( 24 * 3600000u <= uMilSec )
    uMilSec -= 24 * 3600000u;
  if( DevClock.FMilSecs >= uMilSec )
    {
    FController.uCoilVoltageIndex--;
    // 让电压按余弦率下降到设定电压
    float fRatio = (45 - FController.uCoilVoltageIndex) * 2 * 3.14159f / 180;

    uint32_t uCoilVoltCfg = _GetDevCfgReg(REG_COIL_VOLTAGE);
    float fDeltaU = arm_cos_f32(fRatio) * float(INVERTORVOL_MAX - uCoilVoltCfg);
    uint32_t uVoltage = uCoilVoltCfg + fDeltaU;

    BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(uVoltage) );
    
    if( 0 < FController.uCoilVoltageIndex )
      FController.uCoilVolTime = DevClock.FMilSecs;
    else
      FController.uCoilVolTime = 0;
    }
}
//-----------------------------------------------------------------------------
// 线圈保持正常带电
static void _KeepCoilOnDuty()
{

  if( STATE_FALSE != _GetStateReg( REG_ACPWR_RESUME_LATCH ) )
    {
    // 生成事件
    _SetStateReg      ( REG_ACPWR_RESUME_LATCH, STATE_FALSE );
    EVTMGR_AppendEvent( REG_ACPWR_RESUME_LATCH, STATE_FALSE );
    }

#if ENABLE_PASSBY > 0
  // 允许旁路？
  if( 0 == _GetDevCfgReg(REG_PASSBY_EN) )
#endif
    {
    // 启动线圏供电
    _InvertorStartup( TOKEN_BRDSET );

    // 启动继电器
//#ifdef REG_COIL1_FUNC
//      if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
//#endif
//      {
//      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
//      FController.stCoil1 = coilOnDuty;
//      }
//#ifdef REG_COIL1_FUNC
//    else
//      {
//      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
//      FController.stCoil1 = coilStop;
//      }
//#endif

    SetCoilState(COIL_Normal);
    }
#if ENABLE_PASSBY > 0
  else  // 切换到旁路模式
    {
    // 关闭线圏供电
    BoardCtrl_InvertorOff( TOKEN_BRDSET );

    // 旁路状态
    // 调整继电器状态
    // 常闭结点用于旁路
    BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
    FController.stCoil1 = coilPassby;

    SetCoilState(COIL_PassBy);
    }
#endif

  FController.stController = csMonitor;
  FController.timeStart.FDays  = 0;
  FController.timeStart.FMilSecs = 0;

  // 记录状态
  _MarkState();
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
//// 评估线圈的电压
//// 通过测量电池输出电流，评估电圈电压
//static void _EvaluateCoilVoltage()
//{
//  
//  int iIbat = (int)_GetCommonReg( REG_CM_BAT_Ibus );
//  if( iIbat < 200 )   // 200mA是基础供电电流
//    return ;
//  
//  int iIcoil = iIbat - 200;
//  int Vcoil  = BoardCtrl_GetInvertorVoltage();
//  
//}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 计算当前动作的持续时间
static uint32_t _getDuration()
{

  uint32_t uDifmilSec = 0;

  // 计算时间差
  if( FController.timeStart.FDays == DevClock.FDays )
    {
    if( FController.timeStart.FMilSecs < DevClock.FMilSecs )
      uDifmilSec = DevClock.FMilSecs - FController.timeStart.FMilSecs;
    }
  else if( FController.timeStart.FDays + 1 == DevClock.FDays )
    {
    uDifmilSec = 24 * 3600000ul - FController.timeStart.FMilSecs + DevClock.FMilSecs;
    }

  return uDifmilSec;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 正常监视
static void _DoMonitor()
{

  // 梯度超过阀值？
  if( CRITICAL_VOLTAGE_GRADIENT < abs(FController.fGradient) )
    {
    // 电压值在快速变化
    uint32_t uVoltage    = RealTime_Voltage,
             uVolLevel   = FController.uVoltageLevel,
             uVThreshold = Threshold_voltage;

    FController.uLogicTimer = CRITICAL_VOLTAGE_DELAY;

    if( 0 < FController.fGradient && uVoltage < uVThreshold )
      FController.stController = csPowerResume;
    else if( 0 > FController.fGradient && uVoltage < uVolLevel )
      {
      FController.stController = csPowerLoss;
      }
    else
      {
      FController.uLogicTimer = 0;
      }
    }
  else if( 1000 < abs(FController.fGradient) && 
           RealTime_Voltage > Threshold_voltage )
    {
    // 识别电压等级
    FController.uLogicTimer++;
    if( FController.uLogicTimer > 10 )
      {
      FController.uLogicTimer = 0;

      if( RealTime_Voltage > 342 * RATIO_Voltage )
        {
        FController.uVoltageLevel = 380 * RATIO_Voltage;
        }
      else if( RealTime_Voltage > 198 * RATIO_Voltage )
        {
        FController.uVoltageLevel = 220 * RATIO_Voltage;
        }
      }
    }
  else
    FController.uLogicTimer = 0;
  
//  // 线圈在工作状态时，时刻保护电线圈供电和继电器吸合
//  if( coilOnDuty == FController.stCoil1 && 
//      0x01 != (BoardCtrl_GetCoilsControl() & 0x01) )
//    {
//    // 启动线圏供电
//    _InvertorStartup( TOKEN_BRDSET );

//    // 启动继电器
//#ifdef REG_COIL1_FUNC
//    if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
//#endif
//      {
//      BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
//      }
//    }
}
//-----------------------------------------------------------------------------
// 得电过程
static void _DoPowerResumeProc()
{

  if( FController.uLogicTimer > 0 )
    {
    // 消抖延时达到后，启动线圈供电
    FController.uLogicTimer--;
    if( 0 == FController.uLogicTimer )
      {
      FController.timeStart.FDays  = DevClock.FDays;
      FController.timeStart.FMilSecs = DevClock.FMilSecs;

      // 线圈关闭
      _KeepCoilStop( false );

      SetCoilState(COIL_KeppOff);

      // 生成事件
      _SetStateReg      ( REG_ACPWR_RESUME_LATCH, STATE_TRUE );
      EVTMGR_AppendEvent( REG_ACPWR_RESUME_LATCH, STATE_TRUE );
      }
    else
      {
      // 30%定为无压门限
      uint32_t uVoltage    = RealTime_Voltage,
               uVol30Latch = Threshold_voltage_30;
      if( CRITICAL_VOLTAGE_GRADIENT > FController.fGradient &&
          uVol30Latch > uVoltage )
        {
        // 振荡
        // 延时器复位
        FController.uLogicTimer = 0;

        // 关闭线圈供电, 回到监视模式
        _KeepCoilStop( true );
        }
      }
    }
  else if( Days2020Since2000 < FController.timeStart.FDays )
    {
    // 是否达到闭锁时长
    uint32_t uTimeCfg = Threshold_Delay(REG_PWRON_TIME);
    if( _getDuration() > uTimeCfg )
      {
      // 切换线圈到持续
      _KeepCoilOnDuty();
      }
    else
      {
      // 30%定为无压门限
      uint32_t uVoltage    = RealTime_Voltage,
               uVol30Latch = Threshold_voltage_30,
               uVThreshold = Threshold_voltage;
      // 电压下降，且低于无压门限
      if( 0 > FController.fGradient && uVol30Latch > uVoltage )
        {
        // 线圈关闭, 回到监视模式
        _KeepCoilStop( true );
        }
      else
        {
        // 线圈关闭，继续保持闭锁
        _KeepCoilStop( false );
        }
      }
    }
  else if( CRITICAL_VOLTAGE_GRADIENT > FController.fGradient &&
           RealTime_Voltage >= FController.uVoltageLevel )
    {
    // 切换线圈到持续，回到监视模式
    _KeepCoilOnDuty();
    }
  else
    {
    // 关闭线圈供电, 回到监视模式
    _KeepCoilStop( true );
    }
}
//-----------------------------------------------------------------------------
// 失电过程
static void _DoPowerLossProc()
{

  if( FController.uLogicTimer > 0 )
    {
    // 消抖延时达到后，启动线圈供电
    FController.uLogicTimer--;
    if( 0 == FController.uLogicTimer )
      {
      // 启动线圏供电
      _InvertorStartup( TOKEN_BRDSET );

      FController.timeStart.FDays  = DevClock.FDays;
      FController.timeStart.FMilSecs = DevClock.FMilSecs;

      // 记录状态
      _MarkState();

      SetCoilState(COIL_KeppOn);

      // 生成事件
      _SetStateReg      ( REG_ACPWR_LOSS_SUSTAIN, STATE_TRUE );
      EVTMGR_AppendEvent( REG_ACPWR_LOSS_SUSTAIN, STATE_TRUE );
      }
    else
      {
      uint32_t uVoltage  = RealTime_Voltage,
               uVThreshold = Threshold_voltage;
      if( -CRITICAL_VOLTAGE_GRADIENT < FController.fGradient && 
          uVoltage > uVThreshold )
        {
        // 延时器复位
        FController.uLogicTimer = 0;

        // 切换线圈到持续
        _KeepCoilOnDuty();
        }
      }
    }
  else if( Days2020Since2000 < FController.timeStart.FDays )
    {
    // 是否达到供电时长
    uint32_t uTimeCfg = Threshold_Delay(REG_PWROFF_TIME);
    if( _getDuration() > uTimeCfg )
      {
      // 线圈关闭
      _KeepCoilStop( true );

      // 准备关机
      if( 0 != _GetDevCfgReg(REG_AUTO_TURNOFF) )
        {
        FController.stController = csShutDown;

        FController.timeStart.FDays  = DevClock.FDays;
        FController.timeStart.FMilSecs = DevClock.FMilSecs;

        // 记录状态
        _MarkState();

        SetCoilState(COIL_ShutDownDelay);
        }
      }
    else
      {
      uint32_t uVoltage    = RealTime_Voltage,
               uVThreshold = Threshold_voltage;
      // 电压回升，且超过有压门限
      if( 0 < FController.fGradient && uVoltage > uVThreshold )
        {
        // 切换线圈到持续, 回到监视模式
        _KeepCoilOnDuty();
        }
      else
        {
        // 继续维持线圈供电
        //------------------------
        // 启动线圏供电
        _InvertorStartup( TOKEN_BRDSET );
        
        // 启动继电器
//#ifdef REG_COIL1_FUNC
//        if( 1 == _GetDevCfgReg(REG_COIL1_FUNC) )
//#endif
//          {
//          BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_ON) );
//          FController.stCoil1 = coilOnDuty;
//          }
//#ifdef REG_COIL1_FUNC
//        else
//          {
//          BoardCtrl_PowerControl( TOKEN_BRDCTRL_ENCODE(TOKEN_BRDCTRL_CK1, TOKEN_BRDCTRL_OFF) );
//          FController.stCoil1 = coilStop;
//          }
//#endif

        // 记录状态
        _MarkState();

        SetCoilState(COIL_KeppOn);
        }
      }
    }
  else if( -CRITICAL_VOLTAGE_GRADIENT < FController.fGradient &&
           RealTime_Voltage < Threshold_voltage_30 )
    {
    // 关闭线圈供电, 回到监视模式
    _KeepCoilStop( true );
    }
  else
    {
    // 切换线圈到持续, 回到监视模式
    _KeepCoilOnDuty();
    }
}
//-----------------------------------------------------------------------------
static void _DoShutDownProc()
{

  if( Days2020Since2000 < FController.timeStart.FDays )
    {
    // 计算时间差
    uint32_t uDifmilSec = _getDuration();

    // 是否达到关机时长
    uint32_t uTimeCfg = Threshold_Delay(REG_SHUTDOWN_TIME);
    if( uDifmilSec > uTimeCfg )
      {
      _KeepCoilStop( true );

      // 生成事件
      EVTMGR_AppendEvent( REG_EH_POWEROFF, STATE_TRUE );
      EVTMGR_SaveEvent();

      osDelay( 10 );

      // 关闭装置电源
      BoardCtrl_Shutdown(TOKEN_BRDSET);

      SetCoilState(COIL_ShutDown);
      }
    else 
      {
      // 供电恢复？
      uint32_t uVoltage    = RealTime_Voltage,
               uVThreshold = Threshold_voltage;
      if( uVoltage > uVThreshold && 
          CRITICAL_VOLTAGE_GRADIENT < FController.fGradient )
        {
        // 切换线圈到持续
        _KeepCoilOnDuty();
        }
      }
    }
  else
    {
    _KeepCoilStop( true );

    SetCoilState(COIL_Stop);
    }
}
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
// 控制器定时触发
void CoilCtrl_Tick()
{

  // 每1/4周期计算一次
  FController.uTickCount++;
  if( 0 != (FController.uTickCount % (NUM_SAMPLES_PER_PEROID / INSPECTS_PER_PEROID) ) )
    return ;

  // 计算被监视数据
  _Calculate();

  // 根据状态处理
  switch( FController.stController )
    {
    case csMonitor:
      _DoMonitor();
      break;

    case csPowerLoss:
      _DoPowerLossProc();
      break;

    case csPowerResume:
      _DoPowerResumeProc();
      break;

    case csShutDown:
      _DoShutDownProc();
      break;
    }
    
  // 调整线圈电压
  if( 0 < FController.uCoilVolTime ||
      0 < FController.uCoilVoltageIndex )
    _InvertorrVoltageTune();
    
#ifdef NUM_STEPER
  if( csMonitor == FController.stController )
    iStepPtr = 0;
  else
    {
    if( NUM_STEPER > iStepPtr )
      {
      uint32_t uUSec = DevClock.FMilSecs;
      struct tagStepRecoder *pStep = Steper + iStepPtr;
      uint32_t tm = uUSec / 60000u, // 以分钟为单位,简化计算
               ts = uUSec % 60000u; // 分钟以下的毫秒数
      pStep->ucHour    = tm / 60;
      pStep->ucMin     = tm % 60;
      pStep->ucSec     = ts / 1000;
      pStep->uwMilSec  = ts % 1000;
        
      pStep->uVoltage  = FController.uVoltage;
      pStep->fGradient = FController.fGradient;
      pStep->stCtrl    = FController.stController;
      pStep->stCoil1   = FController.stCoil1;
      }
    iStepPtr++;
    }
#endif

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

//  // 有压指示灯
//  if( RealTime_Voltage > Threshold_voltage )
//    SetMLEDState(RMLED_Voltage);
//  else
//    ClrMLEDState(RMLED_Voltage);
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
    FController.uVoltageLevel = 220 * RATIO_Voltage;

  //----------------------------------
  TDevClock  timeStart;
  timeStart.FDays  = RTC_ReadCtrlState(INDEX_START_DATE);
  timeStart.FMilSecs = RTC_ReadCtrlState(INDEX_START_TIME);

  TControlerState stCtrl = (TControlerState)RTC_ReadCtrlState(INDEX_CTRL_STATE);

  if( Days2020Since2000 < timeStart.FDays )
    {
    int iDifmilSec = -1;
    // 计算时间差
    if( timeStart.FDays == DevClock.FDays )
      {
      if( timeStart.FMilSecs < DevClock.FMilSecs )
        iDifmilSec = DevClock.FMilSecs - timeStart.FMilSecs;
      }
    else if( timeStart.FDays + 1 == DevClock.FDays )
      {
      iDifmilSec = 24 * 3600000ul - timeStart.FMilSecs + DevClock.FMilSecs;
      }

    // 根据保存的状态，判定是否还需要继续控制
    switch( stCtrl )
      {
      case csPowerLoss:
        {
        uint32_t uTimeCfg = Threshold_Delay(REG_PWROFF_TIME);
        if( 0 < iDifmilSec && iDifmilSec < uTimeCfg )
          {
          // 在动作过程中复位，继续
          FController.timeStart.FDays  = timeStart.FDays;
          FController.timeStart.FMilSecs = timeStart.FMilSecs;

          FController.stController = stCtrl;
          FController.stCoil1      = (TCoilState)RTC_ReadCtrlState(INDEX_COIL1_STATE);

          _InvertorStartup( TOKEN_BRDSET );
          }
        break;
        }

      case csPowerResume:
        {
        uint32_t uTimeCfg = Threshold_Delay(REG_PWRON_TIME);
        if( 0 < iDifmilSec && iDifmilSec < uTimeCfg )
          {
          // 在动作过程中复位，继续
          FController.timeStart.FDays  = timeStart.FDays;
          FController.timeStart.FMilSecs = timeStart.FMilSecs;
            
          FController.stController = stCtrl;
          FController.stCoil1      = (TCoilState)RTC_ReadCtrlState(INDEX_COIL1_STATE);

          BoardCtrl_InvertorOff( TOKEN_BRDSET );
          }
        break;
        }

      default:
        {
        _KeepCoilOnDuty();

        break;
        }
      }
    }

  // 等待20ms，让采样稳定
  osDelay( 20 );
  FController.uVoltage  = RealTime_Voltage;

  // 进入正常监视模式，清理失电记忆信息
  if( csMonitor == FController.stController )
    {
    FController.timeStart.FDays    = 0;
    FController.timeStart.FMilSecs = 0;

    _MarkState();
    }

  // 若是冷启动，则直接进入上电闭锁状态
  if( GetRSTSrc(RRS_PORRST) )  // 冷启动
    {
    FController.stController = csPowerResume;
    FController.uLogicTimer = 1;
    }
  else
    {
    // 其它原因重启

    // 保持线圈有电
    _KeepCoilOnDuty();
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
