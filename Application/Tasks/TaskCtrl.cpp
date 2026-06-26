//-----------------------------------------------------------------------------
/*
  File        : TaskCTRL.cpp
  Version     : V1.10
  By          : 银网科技

  Description : 控制器（Controller）任务

  Date        : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "TaskCtrl.h"

#include "NumProc.h"
#include "CoilCtrl.h"
#include "BoardCtrl.h"

#include "rtc.h"
#include "dio.h"
#include "DevRegs.h"

#include <cmsis_os.h>
#include <string.h>
#include <stdio.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据申明
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 采样计算中断信号
///* Definitions for sgBinarySemCtrl */
//osSemaphoreId_t sgBinarySemCtrlHandle = nullptr;

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// Controller任务
static void taskController(void *argument)
{

  // 初始化线圈管理器
  BoardCtrl_Init();

  //---------------------------------
  // 等待ADC采样完成至少一个周期，有初步采样结果
  while( 0 == GetTodoTask(RTT_COIL_LOGIC) )
    {
    osDelay(5);
      
    // 置任务有效
    SetRSTSrc( RRS_CTRLTASK );
    }
    
  // 识别开机时刻控制器的状态
  CoilCtrl_Startup();

  // 置任务有效
  SetRSTSrc( RRS_CTRLTASK );

  osDelay( 1 );

  //---------------------------------
  // 开始任务循环
  auto uNextTikc = RTC_GetSysMsTick() + 250;
  while( true )
    {
    // 在条件具备时执行线圈控制逻辑
    if( STATE_FALSE != _GetDevCfgReg(REG_AUTOCTRL_EN) && 
        0 !=  GetTodoTask(RTT_COIL_LOGIC) )
      {
      // 线圈控制
      CoilCtrl_Tick();

      ClrTodoTask(RTT_COIL_LOGIC);
      }

    // 置任务有效
    SetRSTSrc( RRS_CTRLTASK );

    //--------------------------------- 
    // 扫描遥信
    DISCAN_SignalScan();

    // 当前时刻
    auto uMsTick = RTC_GetSysMsTick();
      
    // 定时调整输出电压
    if( uNextTikc <= uMsTick )
      {
      // 置任务有效
      SetRSTSrc( RRS_CTRLTASK );

      // 母板控制
      BoardCtrl_Tick();

      uNextTikc = uMsTick + 250;
      }

    // 置任务有效
    SetRSTSrc( RRS_CTRLTASK );

    //
    osDelay( 4 );
    }
}
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
// 创建任务
void CreateCtrlTask()
{

//#if osCMSIS >= 0x20000U
//  static constexpr osSemaphoreAttr_t sgBinarySemCtrl_attributes = 
//    {
//    .name = "sgBinarySemCtrl"
//    };
//  sgBinarySemCtrlHandle = osSemaphoreNew(1, 1, &sgBinarySemCtrl_attributes);
//#else
//  osSemaphoreDef(Sim_Controller);
//  SimCtrlMsg = osSemaphoreCreate( osSemaphore(Sim_Controller) , 1 );
//#endif

#if osCMSIS >= 0x20000U
  static constexpr osThreadAttr_t ctrlTask_attributes = 
    {
    .name       = "CTRLTask",
    .stack_size = 2048 * 4,
    .priority   = (osPriority_t) osPriorityHigh, //osPriorityRealtime,
    };
  osThreadNew(taskController, NULL, &ctrlTask_attributes);
#else
  osThreadDef((char*)taskCTRL, taskController, osPriorityRealtime, 0, 1024);
  osThreadCreate(osThread(taskCTRL), 0);
#endif
}
//-----------------------------------------------------------------------------
// 控制器触发
// 如果从中断中调用本方法，则该中断的优先级不得<5
void ControllerTick(void)
{

//  if( nullptr != sgBinarySemCtrlHandle )
//    osSemaphoreRelease( sgBinarySemCtrlHandle );

  // 允许线圈控制
  SetTodoTask( RTT_COIL_LOGIC );
}
//-----------------------------------------------------------------------------
