//-----------------------------------------------------------------------------
/*
 File        : TaskApp.c
 Version     : V1.10
 By          : 银网科技

 Description :后台任务

 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "TaskApp.h"

#include "TaskHMI.h"
#include "TaskUart.h"
#include "TaskCtrl.h"

#include "crc.h"
#include "dma.h"
#include "dio.h"
#include "fsmc.h"
#include "gpio.h"
#include "iwdg.h"
#include "rtc.h"
#include "tim.h"
#include "RNGen.h"
#include "SPIChls.h"
#include "IndLED.h"

#include "ADCMgr.h"
#include "DevRegs.h"
#include "DevIntf.h"
#include "DevEvtMgr.h"
#include "NumProc.h"

#include "WaveLogger.h"

#include "CRC1632.h"
#include "DevClock.h"

#include <cmsis_os.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 检查上电原因
static void markResetSource(void)
{

  // 在检查复位方式，并记录
  ClrRSTSrc( -1ul );
    
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)  != RESET )
    SetRSTSrc(RRS_IWDGRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)  != RESET )
    SetRSTSrc(RRS_WWDGRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)   != RESET )
    SetRSTSrc(RRS_PORRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)   != RESET )
    SetRSTSrc(RRS_PINRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)   != RESET )
    SetRSTSrc(RRS_SFTRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)   != RESET )
    SetRSTSrc(RRS_BORRST);
  if( __HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)  != RESET )
    SetRSTSrc(RRS_LPWRRST);

  // Clear reset flags
  __HAL_RCC_CLEAR_RESET_FLAGS();
}
//-----------------------------------------------------------------------------
// 启动外设
static void _StartDevice()
{

  // 存贮空间
  DevReg_Init();

  // 标记复位源
  markResetSource();

  // 初始化功能接口
  DevIntf_Init();

  // 事件
  EVTMGR_Init();

  // 指示灯
  INDLED_Init();

  // 初始化硬件外设
  // Initialize all configured peripherals
  MX_GPIO_Init();

  MX_CRC_Init();

  MX_DMA_Init();

  // SPI 
  SPIChl_Init();
  SPIChl_OSSim();
  
  MX_TIMs_Init();

  MX_FSMC_Init();

  // 
  RNG_Init( HAL_GetTick() );

  // 必须在RTC前初始化，
  DEVCLK_Init( &DevClock );

  // 
  MX_RTC_Init();

  //
  DISCAN_Init();

  //--------------------
  // 装置参数
  FIX_Init();

#if WAVELOGGER_EN > 0
  // 准备录波器  
  WAVELOG_Init();
#endif
  
  // ADC采样
  ADCMGR_Init();
  // ADC管理器
  ADCMGR_Start();
  
  // 设备自检异常
  if( 0 != GetHWFault(RHF_LED_MASK) || 0 != GetFXState(0x000F) )
    SetMLEDState( RMLED_Fault );
  else
    ClrMLEDState( RMLED_Fault );
}
//-----------------------------------------------------------------------------
// 根据装置状态，确定当前 状态LED 的闪烁节奏
static uint32_t GetWKLED_Parten(void)
{
  
  uint32_t uRes;
  switch( GetDevMode )
    {
    case RDM_NORMAL:
      {
      uRes = 0xF0;
      break;        
      }
      
    case RDM_TEST:
      {
      uRes = 0xC0;
      break;        
      }
      
    case RDM_HWFAULT:
      {
      uRes = 0xEA;
      break;
      }
      
    case RDM_FIXERROR:
      {
      uRes = 0xE4;
      break;
      }
      
    default:
      {
      uRes = 0xAA;
      break;
      }
    }

  return uRes;
}
//-----------------------------------------------------------------------------
// 更新面板上的LED指示灯
static void _UpdatePanelLEDs()
{

  // 更新故障指示灯
  if( 0 != GetHWFault(RHF_LED_MASK) )
    {
    SetMLEDState( RMLED_Fault );
    }
  else
    {
    ClrMLEDState( RMLED_Fault );
    }
    
  // 更新通讯灯
#if (VER_SG12B10 >= 120)
  if( 0 != GetCLEDState(RCLED_UART_Tx) )
    {
    SetMLEDState(RMLED_CommTx);
    }
  else
    {
    ClrMLEDState(RMLED_CommTx);
    }
#endif
  ClrCLEDState( -1ul );      // 清通讯状态

  // Update Panel LEDs
  INDLED_SetLamps( RML_MLED_REG );
}
//-----------------------------------------------------------------------------
// 执行待办任务
static void Do_TodoTasks()
{
  
  if( GetTodoTask(RTT_SAVE_DCFG) )
    {
    FIX_SaveDevConfig( 1 );
      
    ClrTodoTask(RTT_SAVE_DCFG);
      
    if( GetTodoTask(RTT_RST_SAVEDEVCFG) )
      {
      HAL_NVIC_SystemReset();
      
      while(1) __NOP();
      }
    }

  // 保存事件记录到NvRAM
  if( RTT_SAVE_EVENT == GetTodoTask(RTT_SAVE_EVENT) )
    EVTMGR_SaveEvent();  // 保存事件记录
  
#if WAVELOGGER_EN > 0
  if( GetTodoTask(RTT_SAVE_WAVLOG) )
    WAVELOG_Save();
#endif    
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// appTask function
static void appTask(void *argument)
{

  (void)argument;
  
  // 启动外设
  _StartDevice();
  
  // definition and creation of mmiTask
  CreateHMITask();
  
  // 锁定板件
  uint32_t uHWId = CRC32( (void*)UID_BASE, 12, -1ul );
  if( 0x6ECDDDD6 == uHWId ) //|| 0x80BFA01F == uHWId )
    {
    // 控制器任务
    //CreateCtrlTask();

    //osDelay( 10 );
      
    //CreateUARTTask();

    // 启动看门狗
    MX_IWDG_Init();
    }
//  else
//    {
//    SetDevMode( RDM_UNKWNHWID );
//    }
    
  // 发送“装置上电事件”
  if( GetRSTSrc(RRS_IWDGRST) )
    EVTMGR_AppendEvent( REG_EH_POWERON_WDG, STATE_TRUE );
  else if( GetRSTSrc(RRS_SFTRST) )
    EVTMGR_AppendEvent( REG_EH_POWERON_SFT, STATE_TRUE );
  else if( GetRSTSrc(RRS_PORRST) )
    EVTMGR_AppendEvent( REG_EH_POWERON_PWR, STATE_TRUE );
  else
    EVTMGR_AppendEvent( REG_EH_POWERON, STATE_TRUE );
  
  // 呼吸灯的状态
  uint32_t uWkPrtn = GetWKLED_Parten(), uWkMask = 1;
  uint32_t uLastLampTick = HAL_GetTick(),
           uLastADCTick  = uLastLampTick,
           uLastRTCTick  = uLastLampTick,
           uLastMeaTick  = uLastLampTick;
  uint8_t  ucSyncClock   = 0;
  uint32_t uTaskTicks = 10;

  // Infinite loop
  while( 1 )
    {
    // v2.3
    // Wey 2025.8.12
    // 灵活调整空闲延时
    if( 10 <= uTaskTicks )
      uTaskTicks = 1;
    else
      uTaskTicks = 10 - uTaskTicks;
    
    osDelay(uTaskTicks);
    
    // 取当前时间
    uint32_t uTick = HAL_GetTick();
    
    // 更新呼吸灯状态
    if( uTick - uLastLampTick >= 160 )
      {
      // 更新计时
      uLastLampTick = uTick;

      // 更新呼吸灯
      if( (uWkPrtn & uWkMask) )
        {
        SetMLEDState(RMLED_State);
        }
      else
        {
        ClrMLEDState(RMLED_State);
        }
              
      uWkMask <<= 1;
      // 更新呼吸灯闪烁方式
      if( 0x100 == uWkMask )
        {
        uWkMask = 1;
          
        uWkPrtn = GetWKLED_Parten();
        }
      
        // 更新面板上的LED指示灯
        _UpdatePanelLEDs();

      // 上电0.16秒后进入正常状态
      if( RDM_INIT == GetDevMode )
        {
        SetDevMode( RDM_NORMAL );
        }
      }

    // v2.3
    // Wey 2025.8.12
    // 1.增加置RRS_APPTASK频率，防止IWDT超时
    // 2.将测量量计算优先
    // 3.将各项数据采集调用互斥，避免集中占用任务周期的时间
    // 4.空闲状态下，执行todolist
    // 置任务有效
    SetRSTSrc( RRS_APPTASK );

    // 每秒计算3次测量数据
    if( uTick - uLastMeaTick >= 333 )
      {
      uLastMeaTick = uTick;

      // 计算测量数据
      NumProc_MeasureCalc();
      }

    // 直流测量
    else if( uTick - uLastADCTick >= 1000 )
      {
      uLastADCTick = uTick;
        
      ADCMDR_ReadBatMeasure();
      }
      
    // 从RTC中读温度和电池电压
    // 每60s更新一次
    else if( uTick - uLastRTCTick >= 60000 )
      {
      // 每5分钟同步一次
      if( ++ucSyncClock >= 5 )
        {
        // 用外RTC同步内部时钟
        if( HAL_OK == RTC_ExRTCSync() )
          ucSyncClock = 0;
        }
      // v2.3 把【时钟同步】和【读RTC内温度】分开
      else
        {
        uLastRTCTick = uTick;
        
        ADCMDR_ReadRTCMeasure();
        }
      }
    else
      // 空闲时执行
      // 执行计划任务
      Do_TodoTasks();

    // 置任务有效
    SetRSTSrc( RRS_APPTASK );

    // 计算本次任务循环的耗时
    uTaskTicks = HAL_GetTick();
    if( uTaskTicks >= uTick )
      uTaskTicks = uTaskTicks - uTick;
    else
      uTaskTicks = (-1ul - uTick) + uTaskTicks;
    }
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Tasks_Init(void) 
{
  
  // USER CODE BEGIN RTOS_MUTEX
  // 
  SPIChl_OSSim();

#if osCMSIS >= 0x20000U
  static const osThreadAttr_t appTask_attributes = 
    {
    .name = "appTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t) osPriorityNormal,
    };
  osThreadNew(appTask, NULL, &appTask_attributes);
#else
  // definition and creation of masterTask
  osThreadDef(masterTask, TaskApplication, osPriorityNormal, 0, 1024);
  osThreadCreate(osThread(masterTask), NULL);
#endif
}
//-----------------------------------------------------------------------------
