//-----------------------------------------------------------------------------
/*
 File        : DevRegs.h
 Version     : V1.10
 By          : 银网科技

 Description :定义系统寄存器  存贮空间、分配寄存器地址、访问方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef DEV_REGISTER_H
#define DEV_REGISTER_H

#include "DevBuffer.h"
#include "DevFixed.h"
#include "Dev_Cfg.h"
#include "DevTypes.h"

#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 操作令牌
//-----------------------------------------------------------------------------
#define TOKEN_EDIT_SET            0xAA33CC55
//=============================================================================
// 寄存器边界标识
//-----------------------------------------------------------------------------
#define BORDER_SIGN_DW            0x5A3CC3A5
#define BORDER_SIGN_B             0xAC
//=============================================================================
// 寄存器地址分配
//-----------------------------------------------------------------------------
// 寄存器分类
#define REG_TYPE(x)               ((x) & 0xFFFFF000)
// 寄存器序号
#define REG_INDEX(x)              ((x) & 0x00000FFF)
//-----------------------------------------------------------------------------
// 设备的配置(TDeviceFixed）
#define REG_DEVOPTION             0x00001000
#define REG_DEVCONFIG             0x00002000
#define REG_DIGINALAIS            0x00003000

// 设备的校准系数(TDeviceFixed）
#define REG_CALIBRATION           0x00004000

// 设备的保护压板(DevHolding）
#define REG_SWITCH                0x00005000
#define REG_SWITCHEDIT            0x00006000

// 设备的保护定值(DevHolding）
#define REG_HOLDING               0x00007000
#define REG_HOLDINGEDIT           0x00008000
//-----------------------------------------------------------------------------
// 固件工作状态（DevCache.DeviceState）
#define REG_DEVSTATE              0x00009000

// IO状态（DevCache.DeviceState）
#define REG_IOSTATE               0x0000A000

// 状态寄存器(DevCache.State）
#define REG_STATE                 0x0000B000

// 通用寄存器（DevCache.Common)
#define REG_COMMON                0x0000C000

// 浮点寄存器（DevCache.Common)
#define REG_REAL                  0x0000D000
//-----------------------------------------------------------------------------
// 事件
#define REG_EVENT                 0x0000E000
//-----------------------------------------------------------------------------
// 功能寄存器
#define REG_FUNCTION              0x0000F000
//-----------------------------------------------------------------------------
// 以后为装置测试用功能寄存器
// 传动实验
#define REG_TEST                  0x00010000        
#define REG_MEASADJ               0x00010000
#define REG_MEASTEST              0x00011000
#define REG_DITEST                0x00012000
#define REG_DOTEST                0x00013000
//=============================================================================
// 设备的配置(TDeviceFixed.Options）
//-----------------------------------------------------------------------------
// [设备配置]可以读，但不可以通过寄存器写
// 编辑[设备配置]应从对应的REG_FN_xxx寄存器访问
#define REG_PROTECT_EN            (REG_DEVOPTION + 0)     // 保护总压板
#define REG_SRLY_AUTORET          (REG_DEVOPTION + 1)     // 信号继电器自动复归
#define REG_CT_1A                 (REG_DEVOPTION + 2)     // 1A CT
#define REG_VOLTAGE_33            (REG_DEVOPTION + 3)     // 3相3线接线方式
  #define GetDevOption(n)         ((DevConfig.Options & (1 << ((n) - REG_DEVOPTION))) != 0)

#define REG_DEVOPT_USER           (REG_DEVOPTION + 1)     // 用户可自行定义的寄存器
//=============================================================================
// 设备参数(TDeviceFixed.Configs）
//-----------------------------------------------------------------------------
// [设备参数]可以读，但不可以通过寄存器写
// 编辑[设备参数]应从对应的REG_FN_xxx寄存器访问
#define REG_DEV_ADDR              (REG_DEVCONFIG + 0)     // 装置地址
#define REG_WAVELOG_PRE           (REG_DEVCONFIG + 1)     // 录波器预录周期

#define REG_AUTOCTRL_EN           (REG_DEVCONFIG + 3)     // 允许自动控制
#define REG_AUTO_TURNOFF          (REG_DEVCONFIG + 4)     // 自动关机
#define REG_AUTO_BREAKER_ON       (REG_DEVCONFIG + 5)     // 允许自动合闸
#define REG_PASSBY_EN             (REG_DEVCONFIG + 6)     // 允许旁路运行
#define REG_ACTION_VOLTAGE        (REG_DEVCONFIG + 8)    // 线圈投入的电压阀值%
#define REG_COIL_VOLTAGE          (REG_DEVCONFIG + 9)    // 线圈供电电压

#define REG_PWRON_TIME            (REG_DEVCONFIG + 10)    // 得电待持时间s
#define REG_PWROFF_TIME           (REG_DEVCONFIG + 11)    // 失电持续时间s
#define REG_SHUTDOWN_TIME         (REG_DEVCONFIG + 12)    // 自动关机时间s
#define REG_RELAY_DELAY           (REG_DEVCONFIG + 14)    // CK1继电器启动延时s
#define REG_RELAY_TIME            (REG_DEVCONFIG + 15)    // CK1继电器动作时间s

// 用户可自行定义的寄存器
#define REG_DEVCFG_USER           (REG_DEVCONFIG + 25)      // 25
//-----------------------------------------------------------------------------
// 工作参数
#define REG_TESTPARAM              (REG_DEVCFG_USER + 0)
#define REG_TEST_IgniteWait        (REG_TESTPARAM + 0)
#define REG_TEST_Ignite            (REG_TESTPARAM + 1)
#define REG_TEST_IgnitePause       (REG_TESTPARAM + 2)
#define REG_TEST_IgniteCicles      (REG_TESTPARAM + 3)      
#define REG_TEST_BreakerONWait     (REG_TESTPARAM + 6)      
#define REG_TEST_BreakerOFFWait    (REG_TESTPARAM + 7)
#define REG_TEST_IgniteWaitCurrent (REG_TESTPARAM + 8)
#define REG_Test_MagnetBlastWait   (REG_TESTPARAM + 9)
//-----------------------------------------------------------------------------
// 通讯参数直接定义在TDeviceFixed结构中，以提高访问效率
// 这些参数通过功能寄存器访问
// 网口地址参数
#define GetNIC1Config             (DevConfig.ETHConfig[0])
#define GetNIC1SktCfg(n)          (DevConfig.ETHSocketsConfig[n]) 
#if NUM_ETH_PORTS > 1
#define GetNIC2Config             (DevConfig.ETHConfig[MAC_NIC_Beg + 1])
#define GetNIC2SktCfg(n)          (DevConfig.ETHSocketsConfig[n]) 
#define GetNIC3Config             (DevConfig.ETHConfig[MAC_NIC_Beg + 2])
#define GetNIC3SktCfg(n)          (DevConfig.ETHSocketsConfig[n]  
#define GetNIC4Config             (DevConfig.ETHConfig[MAC_NIC_Beg + 3])
#define GetNIC4SktCfg(n)          (DevConfig.ETHSocketsConfig[n]  
#endif

#if defined(NUM_UART_PORTS) && (NUM_UART_PORTS > 0 )
// 串口1参数                      
#define GetUART1Config            (DevConfig.UART1Config)
#endif
//=============================================================================
// 开入别名(TDeviceFixed.DigInAlias)
//-----------------------------------------------------------------------------
// [开入别名]可以读，但不可以通过寄存器写
// 编辑[开入别名]应从对应的REG_FN_xxx寄存器访问
#define REG_DIAlais               REG_DIGINALAIS           // 
#define REG_DIAlais1              (REG_DIAlais + 0)        // 
#define REG_DIAlais2              (REG_DIAlais + 1)        // 
#define REG_DIAlais3              (REG_DIAlais + 2)        // 
#define REG_DIAlais4              (REG_DIAlais + 3)        // 
#define REG_DIAlais5              (REG_DIAlais + 4)        // 
#define REG_DIAlais6              (REG_DIAlais + 5)        // 
#define REG_DIAlais7              (REG_DIAlais + 6)        // 
#define REG_DIAlais8              (REG_DIAlais + 7)        // 
#define REG_DIAlais9              (REG_DIAlais + 8)        // 
#define REG_DIAlais10             (REG_DIAlais + 9)        // 
#define REG_DIAlais11             (REG_DIAlais + 10)       // 
//=============================================================================
// 校准系数(TDeviceFixed.Calib）
//-----------------------------------------------------------------------------
//   A.系数的表示：
//         0: 不准正
//     16384: =1
//   B.系数的计算：
//       1.用于模值
//                  标定值 x 16384
//         calib = ----------------- 
//                      输入值
//       2.用于角度
//         calib =  (标定值 - 输入值)
//   C.系数的使用
//       1.用于模值
//                     输入值 x calib
//         校正结果 = -----------------
//                         16384
//       2.用于角度
//         校正结果= (输入值 + calib)
//-----------------------------------------------------------------------------
// 交流输入模值系数
// 模值校正系数
#define REG_ACModCalib            (REG_CALIBRATION + 0)  //  0
#define REG_ACMod0Calib           (REG_ACModCalib + 0)   //
#define REG_ACMod1Calib           (REG_ACModCalib + 1)   //
#define REG_ACMod2Calib           (REG_ACModCalib + 2)   //
#define REG_ACMod3Calib           (REG_ACModCalib + 3)   //
  #define GetACModCalib(n)        (DevConfig.CaliCoef[(n) - REG_ACModCalib])
//=============================================================================
// 工作状态寄存器(DevCache.DeviceState）
//-----------------------------------------------------------------------------
#define REG_DEV_MODE              (REG_DEVSTATE + 0)      // 装置的工作模式
  #define RDM_NORMAL              0                       // 正常工作
  #define RDM_INIT                0x1111                  // 正在起动  
  #define RDM_CALIB               0x3333                  // 校准状态
  #define RDM_TEST                0x5555                  // 正在试验中
  #define RDM_SETCFG              0xA5A5                  // 正在维护参数
  #define RDM_HWFAULT             0xAAAA                  // 硬件故障
  #define RDM_FIXERROR            0xCCCC                  // 定值错误
  #define RDM_UNKWNHWID           0xEDED                  // 硬件ID非法
  #define SetDevMode(e)           (DevCache.DeviceState[REG_DEV_MODE - REG_DEVSTATE] = (e))
  #define GetDevMode              (DevCache.DeviceState[REG_DEV_MODE - REG_DEVSTATE])

#define REG_RST_SOURCE            (REG_DEVSTATE + 1)      // 装置的上电方式
  #define RRS_PORRST              (1 << 0)                // 上电复位（冷启动）
  #define RRS_PINRST              (1 << 1)                // nRST管脚
  #define RRS_SFTRST              (1 << 2)                // 软件重启指令
  #define RRS_BORRST              (1 << 3)                // 供电电压低于阀值产生的复位
  #define RRS_IWDGRST             (1 << 4)                // IWDG
  #define RRS_WWDGRST             (1 << 5)                // WWDG
  #define RRS_LPWRRST             (1 << 6)                // 电源电压低-复位
  #define RRS_APPTASK             (1 << 8)                // APP任务有效
  #define RRS_ADCTASK             (1 << 9)                // ADC任务有效
  #define RRS_TMRTASK             (1 << 10)               // Timer任务有效
  #define RRS_GUITASK             (1 << 11)               // GUI任务有效
  #define RRS_UARTTASK            (1 << 12)               // UART任务有效
  #define RRS_CTRLTASK            (1 << 13)               // Controller任务有效
  #define SetRSTSrc(e)            ( DevCache.DeviceState[REG_RST_SOURCE - REG_DEVSTATE] |= (e) )
  #define GetRSTSrc(e)            ( DevCache.DeviceState[REG_RST_SOURCE - REG_DEVSTATE] & (e) )
  #define ClrRSTSrc(e)            ( DevCache.DeviceState[REG_RST_SOURCE - REG_DEVSTATE] &= (TDevStateReg)~(e) )

#define REG_HW_FAULT              (REG_DEVSTATE + 2)      // 装置的硬件故障
  #define RHF_RCC_ERR             (1 << 0)                // RCC错误
  #define RHF_LSE_ERR             (1 << 1)                // LSE错误
  #define RHF_LSI_ERR             (1 << 2)                // LSI错误
  #define RHF_FSMC_ERR            (1 << 3)                // 并行总线错误 
  #define RHF_RTC_ERR             (1 << 4)                // RTC初始化失败
  #define RHF_ExRTC_ERR           (1 << 5)                // 外RTC错误
  #define RHF_TIM_ERR             (1 << 6)                // 定时器初始化失败
  #define RHF_SPI_ERR             (1 << 7)                // SPI初始化失败
  #define RHF_ADC_ERR             (1 << 8)                // 内部ADC初始化失败
  #define RHF_ExADC1_ERR          (1 << 9)                // 外部ADC1初始化失败
  #define RHF_ExADC2_ERR          (1 << 10)               // 外部ADC2初始化失败
  #define RHF_Freq_MIS            (1 << 11)               // 频率捕获失败
  #define RHF_UART_ERR            (1 << 12)               // UART初始化失败
  #define RHF_ExFLASH_ERR         (1 << 13)               // 文件存贮器错误
  #define RHF_FeRAM_ERR           (1 << 14)               // 定值存贮器错误
  #define RHF_RHEOSTAT_ERR        (1 << 15)               // 变阻器错误
  #define SetHWFault(e)           ( DevCache.DeviceState[REG_HW_FAULT - REG_DEVSTATE] |= (e) )
  #define ClrHWFault(e)           ( DevCache.DeviceState[REG_HW_FAULT - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetHWFault(e)           ( DevCache.DeviceState[REG_HW_FAULT - REG_DEVSTATE] & (e) )
  #define RHF_LED_MASK            (-1ul)                  // 驱动[异常LED]的硬件故障Bits Mask

#define REG_FIX_STATE             (REG_DEVSTATE + 3)      // 装置的定值错误
  #define RFE_DCFG_ERROR          (1 << 0)                // 装置参数错误，已初始化
  #define RFE_CLIB_ERROR          (1 << 1)                // 校正系数错误
  #define RFE_HOLD_ERROR          (1 << 2)                // 保护定值错误
  #define REF_LOGIDX_ERR          (1 << 3)                // 报告记录错误
  #define REF_DCFG_MDIFD          (1 << 8)                // 装置参数已修改
  #define REF_HSWT_MDIFD          (1 << 9)                // 保护压板已修改
  #define REF_HOLD_MDIFD          (1 << 10)               // 保护定值已修改
  #define REF_SIGN_HOLD           (1 << 15)               // 状态保持标记
  #define SetFXState(e)           ( DevCache.DeviceState[REG_FIX_STATE - REG_DEVSTATE] |= (e) )
  #define ClrFXState(e)           ( DevCache.DeviceState[REG_FIX_STATE - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetFXState(e)           ( DevCache.DeviceState[REG_FIX_STATE - REG_DEVSTATE] & (e) )

#define REG_RTC_STATE             (REG_DEVSTATE + 4)      // 外部RTC状态
  #define EXRTC_RTCF              (1 << 0)                // 芯片上电
  #define EXRTC_OSF               (1 << 1)                // 停振标志位，OSF=1,表示之前有过停振发生
  #define EXRTC_BLF               (1 << 2)                // 电池欠压
  #define EXRTC_BHF               (1 << 3)                // 电池过压
  #define EXRTC_PMF               (1 << 4)                // 当前是电池模式
  #define EXRTC_CHARGE            (1 << 5)                // 充电允许
  #define EXRTC_F32K              (1 << 6)                // 32K输出允许
  #define SetRTCStReg(e)          ( DevCache.DeviceState[REG_RTC_STATE - REG_DEVSTATE]  =  (e) )
  #define GetRTCStReg(e)          ( DevCache.DeviceState[REG_RTC_STATE - REG_DEVSTATE] )
  #define SetRTCState(e)          ( DevCache.DeviceState[REG_RTC_STATE - REG_DEVSTATE] |=  (e) )
  #define ClrRTCState(e)          ( DevCache.DeviceState[REG_RTC_STATE - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetRTCState(e)          ( DevCache.DeviceState[REG_RTC_STATE - REG_DEVSTATE] & (e) )

#define REG_CLED_STATE            (REG_DEVSTATE + 5)      // 通讯指示灯
  #define RCLED_UART_Rx           (1 << 0)                // 串口接收
  #define RCLED_UART_Tx           (1 << 1)                // 串口发送
  #define SetCLEDState(e)         ( DevCache.DeviceState[REG_CLED_STATE - REG_DEVSTATE] |= (e) )
  #define ClrCLEDState(e)         ( DevCache.DeviceState[REG_CLED_STATE - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetCLEDState(e)         ( DevCache.DeviceState[REG_CLED_STATE - REG_DEVSTATE] & (e) )

// 面板通信灯
#define REG_MLED_STATE            (REG_DEVSTATE + 6)      // 面板上的指示灯 16bits 从左到右排
  #define RMLED_State             (1 << 0)                // 运行
  #define RMLED_Fault             (1 << 1)                // 故障
  #define RMLED_Passby            (1 << 2)                // 旁路
  #define RMLED_KeepLoss          (1 << 3)                // 失电保持
  #define RMLED_LatchResume       (1 << 4)                // 得电闭锁
  #define RMLED_CK1On             (1 << 5)                // 出口1继电器动作
  #define RMLED_InvertorOn        (1 << 6)                // 逆变器启动
  #define RMLED_Charging          (1 << 7)                // 正在充电
  #define RMLED_Charged           (1 << 8)                // 充电完成
  #define RMLED_CommTx            (1 << 9)                // 通信发送
  #define SetMLEDState(e)         ( DevCache.DeviceState[REG_MLED_STATE - REG_DEVSTATE] |= (e) )
  #define ClrMLEDState(e)         ( DevCache.DeviceState[REG_MLED_STATE - REG_DEVSTATE] &= (uint32_t)~(e) )
  #define GetMLEDState(e)         ( DevCache.DeviceState[REG_MLED_STATE - REG_DEVSTATE] & (e) )
  #define RML_MLED_REG            ( DevCache.DeviceState[REG_MLED_STATE - REG_DEVSTATE])
  #define RML_COMM_LEDS           (RMLED_CommRx)

#define REG_TODO_TASK             (REG_DEVSTATE + 7)      // 需要在后台完成的任务 (占两个寄存器)
  #define RTT_SAVE_DCFG           (1 << 0)                // 保存装置参数
  #define RTT_SAVE_HOLD           (1 << 1)                // 保存保护定值
  #define RTT_SAVE_EVENT          (1 << 2)                // 保存事件
  #define RTT_DISP_EVENT          (1 << 3)                // 弹出事件窗(在GUI任务中响应)
  #define RTT_SAVE_WAVLOG         (1 << 4)                // 保存录波
  #define RTT_SHOW_WAVLOG         (1 << 5)                // 显示录波界面
  #define RTT_RELAY_TRIP          (1 << 6)                // 驱动继电器(在中断中响应) 
  #define RTT_RELAY_TIMER         (1 << 7)                // 继电器延时返回
  #define RTT_UPDATE_LED          (1 << 8)                // 更新面板指示灯
  #define RTT_SAVE_STATE          (1 << 9)                // 保存持续状态数据
  #define RTT_COIL_LOGIC          (1 << 10)               // 可驱动Coil逻辑控制
  #define RTT_UART1_RECONFIG      (1 << 13)               // 重新复位UART1 
  #define RTT_RST_SAVEDEVCFG      (1 << 14)               // 保存参数后重启
  #define RTT_SET_RHEOSTAT        (1 << 15)               // 设置变阻器
  #define RTT_RegCache            ( *(volatile uint32_t*)&DevCache.DeviceState[REG_TODO_TASK - REG_DEVSTATE] )
  #define SetTodoTask(e)          ( RTT_RegCache |=  (e) )
  #define ClrTodoTask(e)          ( RTT_RegCache &= (uint32_t)~(e) )
  #define GetTodoTask(e)          ( RTT_RegCache & (e) )

#define REG_SET_HW                (REG_DEVSTATE + 8)      // 需要配置的硬件
  #define RSH_UART1               (1 << 0)                // UART1
  #define RSH_UART1_PROTOCOL      (1 << 4)                // UART1规约
  #define RSH_AI                  (1 << 9)                // AI
  #define RSH_DI                  (1 << 10)               // DI
  #define RSH_DO                  (1 << 11)               // DO
  #define SetSetHW(e)             ( DevCache.DeviceState[REG_SET_HW - REG_DEVSTATE] |=  (e) )
  #define ClrSetHW(e)             ( DevCache.DeviceState[REG_SET_HW - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetSetHW(e)             ( DevCache.DeviceState[REG_SET_HW - REG_DEVSTATE] & (e) )

#define REG_GUI_STATE             (REG_DEVSTATE + 9)     // GUI的状态
  #define RGS_SPLASH              (1 << 0)                // 正在封面
  #define RGS_RTEVENT             (1 << 1)                // 实时事件弹窗
  #define RGS_HLDBLKCHED          (1 << 2)                // 定值区已切换
  #define RGS_WAVLOGFORM          (1 << 3)                // 录波弹窗
  #define RGS_OEMCHG_EN           (1 << 4)                // 经销商切换使能
  #define RGS_FUNCHG_EN           (1 << 5)                // 功能切换使能
  #define RGS_LAMPS_TEST          (1 << 6)                // 指示灯测试中
  #define RGS_DEBUG_FAULT         (1 << 10)               // 调试中出现致命失败
  #define SetGUIState(e)          ( DevCache.DeviceState[REG_GUI_STATE - REG_DEVSTATE] |=  (e) )
  #define ClrGUIState(e)          ( DevCache.DeviceState[REG_GUI_STATE - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetGUIState(e)          ( DevCache.DeviceState[REG_GUI_STATE - REG_DEVSTATE] & (e) )

#define REG_EDIT_MODIFIED         (REG_DEVSTATE + 10)     // 编辑器修改的状态
  #define REM_CFG_MODIFIED        (1 << 0)                // 装置参数被修改
//  #define REM_SWT_MODIFIED        (1 << 1)                // 保护压板被修改
//  #define REM_HLD_MODIFIED        (1 << 2)                // 保护定值被修改
//  #define REM_CMX_MODIFIED        (1 << 3)                // 控制矩阵被修改
//  #define REM_FUN_MODIFIED        (1 << 4)                // 装置功能类型被修改
  #define REM_CALIB_MODIFIED      (1 << 5)                // 模拟量校正数据被修改
  #define REM_UART1_MODIFIED      (1 << 6)                // 串口参数被修改
  #define SetEditState(e)         ( DevCache.DeviceState[REG_EDIT_MODIFIED - REG_DEVSTATE] |=  (e) )
  #define ClrEditState(e)         ( DevCache.DeviceState[REG_EDIT_MODIFIED - REG_DEVSTATE] &= (TDevStateReg)~(e) )
  #define GetEditState(e)         ( DevCache.DeviceState[REG_EDIT_MODIFIED - REG_DEVSTATE] & (e) )

#define REG_DEV_STATE_END         (REG_DEVSTATE + 11)

//-----------------------------
#define REG_EDIT_STATE            (REG_DEV_STATE_END + 1)

#define REG_EditHoldBlock         (REG_EDIT_STATE + 0)        // 当前编辑的定值区号
  #define GetEditBlock            (DevCache.DeviceState[REG_EditHoldBlock - REG_DEVSTATE])

#define REG_EditEnable            (REG_EDIT_STATE + 1)        // 编辑使能
  #define GetEditEnable           (STATE_TRUE == DevCache.DeviceState[REG_EditEnable - REG_DEVSTATE])
  #define SetEditEnable           (DevCache.DeviceState[REG_EditEnable - REG_DEVSTATE] = STATE_TRUE)
  #define ClrEditEnable           (DevCache.DeviceState[REG_EditEnable - REG_DEVSTATE] = STATE_FALSE)
  #define SetEditDisable          ClrEditEnable

#define REG_Password1Ok           (REG_EDIT_STATE + 2)        // 口令1正确
  #define GetPassword1Ok          (STATE_TRUE == DevCache.DeviceState[REG_Password1Ok - REG_DEVSTATE])
  #define SetPassword1Ok          (DevCache.DeviceState[REG_Password1Ok - REG_DEVSTATE] = STATE_TRUE)
  #define ClrPassword1Ok          (DevCache.DeviceState[REG_Password1Ok - REG_DEVSTATE] = STATE_FALSE)

#define REG_Password2Ok           (REG_EDIT_STATE + 3)        // 口令2正确
  #define GetPassword2Ok          (STATE_TRUE == DevCache.DeviceState[REG_Password2Ok - REG_DEVSTATE])
  #define SetPassword2Ok          (DevCache.DeviceState[REG_Password2Ok - REG_DEVSTATE] = STATE_TRUE)
  #define ClrPassword2Ok          (DevCache.DeviceState[REG_Password2Ok - REG_DEVSTATE] = STATE_FALSE)

#define REG_PasswordxOk           (REG_EDIT_STATE + 4)        // 口令x正确(工厂口令)
  #define GetPasswordxOk          (STATE_TRUE == DevCache.DeviceState[REG_PasswordxOk - REG_DEVSTATE])
  #define SetPasswordxOk          (DevCache.DeviceState[REG_PasswordxOk - REG_DEVSTATE] = STATE_TRUE)
  #define ClrPasswordxOk          (DevCache.DeviceState[REG_PasswordxOk - REG_DEVSTATE] = STATE_FALSE)

#define REG_CTRL_STATE            (REG_EDIT_STATE + 5)
#define REG_Rly_RmtCtrl           (REG_CTRL_STATE + 0)        // 当前遥控序号(1~N)
  #define SetRlyRmtCtrl(n)        (DevCache.DeviceState[REG_Rly_RmtCtrl - REG_DEVSTATE] = (n))
  #define GetRlyRmtCtrl()         (DevCache.DeviceState[REG_Rly_RmtCtrl - REG_DEVSTATE] )

#define REG_COIL_STATE            (REG_CTRL_STATE + 1)        // 线圈控制状态
  #define COIL_Stop               0
  #define COIL_Startup            0x33
  #define COIL_Monitor            0x35
  #define COIL_KeepOn             0x53
  #define COIL_KeepOff            0x5A
  #define COIL_Passby             0xA5
  #define COIL_ShutDownDelay      0xC5
  #define COIL_ShutDown           0xCC
  #define COIL_Fatal              0xE0
  #define SetCoilState(n)         (DevCache.DeviceState[REG_COIL_STATE - REG_DEVSTATE] = (n))
  #define GetCoilState(n)         (DevCache.DeviceState[REG_COIL_STATE - REG_DEVSTATE])

#define REG_Persisent             (REG_CTRL_STATE + 2)        // 持续状态，掉电维持
  #define RPS_FaultLED            (1 << 0)                    // 故障指示灯
  #define RPS_Relay1              (1 << 1)                    // 信号继电器1
  #define RPS_Relay2              (1 << 2)                    // 信号继电器2
  #define SetPersisent(x)         (DevCache.DeviceState[REG_Persisent - REG_DEVSTATE] |=  (x))
  #define ClrPersisent(x)         (DevCache.DeviceState[REG_Persisent - REG_DEVSTATE] &= (TDevStateReg)~(x))
  #define GetPersisent(x)         (DevCache.DeviceState[REG_Persisent - REG_DEVSTATE] &(x))

// REG_DEVSTATE数量安全
#define REG_DEVSTATE_END          (REG_CTRL_STATE + 3)
#if ((REG_DEVSTATE_END - REG_DEVSTATE) >= NUM_DEVICE_STATES)
  #error Number REG_DEVSTATE is overflow!
#endif

// 以上状态寄存器
//   1. 不进入寄存器列表
//   2. 不允许通过 DevReg_Read/_GetStateReg读
//=============================================================================
// 状态寄存器(DevCache.State）
//-----------------------------------------------------------------------------
// 值被限定为STATE_TRUE/FALSE
#define REG_DigInp                (REG_IOSTATE + 0)       // 遥信输入
// 本体上的遥信
#define REG_DI0                   (REG_DigInp + 0)        // MCU电源
#define REG_DI1                   (REG_DigInp + 1)        // 充电器电源
#define REG_DI2                   (REG_DigInp + 2)        // 逆变器反馈
#define REG_DI3                   (REG_DigInp + 3)        // 逆变器使能
#define REG_DI4                   (REG_DigInp + 4)        // 合闸状态
#define REG_DI5                   (REG_DigInp + 5)        // 旁路状态
#define REG_DI6                   (REG_DigInp + 6)        // 加热器
#define REG_DI7                   (REG_DigInp + 7)        // 风扇状态
 #define REG_stAdapterOk           REG_DI1
 #define REG_stInvertorOk          REG_DI2
 #define REG_stInvertorEn          REG_DI3
 #define REG_stBreakerON           REG_DI4
 #define REG_stPassby              REG_DI5
 #define REG_stHeater              REG_DI6
 #define REG_stFan                 REG_DI7
  
#define REG_RelayState            (REG_DigInp + NUM_DigInputs)  // 开出状态
#define REG_RELAY0                (REG_RelayState + 0 )         // CK1-合闸
#define REG_RELAY1                (REG_RelayState + 1 )         // Passby
#define REG_RELAY2                (REG_RelayState + 2 )         // Heater
#define REG_RELAY3                (REG_RelayState + 3 )         // Fan

// REG_DEVSTATE数量安全
#define REG_IOSTATE_END           (REG_IOSTATE_END + 6)
#if ((REG_IOSTATE_END - REG_IOSTATE) >= NUM_IO_STATES)
  #error Number NUM_IO_STATES is overflow!
#endif

//=============================================================================
// 状态寄存器
//-----------------------------------------------------------------------------

// 用户可使用的状态寄存器                               
#define REG_STATE_USER            (REG_STATE + 20)           

#define REG_CTRL_LOGIC            (REG_STATE_USER + 0)
#define REG_ACPWR_RESUME_LATCH    (REG_CTRL_LOGIC + 0)
#define REG_ACPWR_LOSS_SUSTAIN    (REG_CTRL_LOGIC + 1)
#define REG_AUTO_CLOSE_BREAKER    (REG_CTRL_LOGIC + 2)

//=============================================================================
// 通用寄存器
//-----------------------------------------------------------------------------
// 边界
#define REG_CRBorder              (REG_COMMON + 0)        // 防越界标识

// 空闲计数
#define REG_IDLE_CNTR             (REG_COMMON + 1)        // 空闲操作计数
  #define IdleCounterTick         (DevCache.Common[REG_IDLE_CNTR - REG_COMMON]++) 
  #define IdleCntrOverTime(t)     (DevCache.Common[REG_IDLE_CNTR - REG_COMMON] >= (t))

// 系统参数                       
#define REG_SYSCLKFreq            (REG_COMMON + 2)        // 系统时钟频率 Hz
#define REG_TIMCLKFreq            (REG_COMMON + 3)        // 系统时钟频率 Hz

// 固件校验
#define REG_FWCRC32               (REG_COMMON + 4)       // 固件CRC

// 调试软件写入令牌
#define REG_INSPTOKEN             (REG_COMMON + 5)       // 

// 各通讯口的新事件标志
#define REG_NEW_EVENTS            (REG_COMMON + 6)       //
  #define GetNewEventSign(x)      (DevCache.Common[REG_NEW_EVENTS - REG_COMMON] &   (x))
  #define ClrNewEventSign(x)      (DevCache.Common[REG_NEW_EVENTS - REG_COMMON] &= ~(x))
  #define SetNewEvents(x)         (DevCache.Common[REG_NEW_EVENTS - REG_COMMON] =   (x))

// 装置状态数据                   
#define REG_DEV_Probe             (REG_COMMON + 10)       // 10
#define REG_DEV_Probe1            (REG_DEV_Probe + 0)     // 
#define REG_DEV_Probe2            (REG_DEV_Probe + 1)
#define REG_DEV_Probe3            (REG_DEV_Probe + 2)
#define REG_DEV_Probe4            (REG_DEV_Probe + 3)

// 继电器定时器
#define REG_RLY_Timer             (REG_COMMON + 16)       // 16
#define REG_CK1_Timer             (REG_RLY_Timer + 0)

// 用户可使用的通用寄存器
#define REG_COMMON_USER           (REG_COMMON + 20)      // 50
// 定时器
#define  REG_CM_TIMERS            (REG_COMMON_USER + 0)
#define  REG_CM_TMR_STARTUP       (REG_CM_TIMERS + 0)
#define  REG_CM_TMR_HEATEROFF     (REG_CM_TIMERS + 1)
#define  REG_CM_TMR_HEATERON      (REG_CM_TIMERS + 2)
#define  REG_CM_TMR_FANOFF        (REG_CM_TIMERS + 3)
#define  REG_CM_TMR_FANON         (REG_CM_TIMERS + 4)

//=============================================================================
// 浮点寄存器
//-----------------------------------------------------------------------------
// 边界
#define REG_RLBorder              (REG_REAL + 0)        // 防越界标识

// RTC中的测量
#define REG_RL_RTC                (REG_REAL + 1)        // 1
#define REG_RL_RTC_TEMP           (REG_RL_RTC + 0)      // 设备温度
#define REG_RL_RTC_Vbat           (REG_RL_RTC + 1)      // RTC电池电压

// 频率
#define REG_RL_FREQ               (REG_REAL + 4)        // 4
// 交流输入频率(电网)  每组频率的3个数据的存贮顺序不能变（ADCMgr.c）
#define REG_RL_ACPeriod           (REG_RL_FREQ + 0)     // 交流输入周期(us)
#define REG_RL_ACFreq             (REG_RL_FREQ + 1)     // 交流输入频率(0.001Hz)
#define REG_RL_ACFreqRT           (REG_RL_FREQ + 2)     // 交流输入频率(0.0001Hz) 实时

// 交流输出频率(逆变器)
#define REG_RL_VOPeriod           (REG_RL_FREQ + 3)     // 交流输出周期(us)
#define REG_RL_VOFreq             (REG_RL_FREQ + 4)     // 交流输出频率(0.001Hz)
#define REG_RL_VOFreqRT           (REG_RL_FREQ + 5)     // 交流输出频率(0.0001Hz) 实时

// 锂电池工作参数
#define REG_RL_BATTERY            (REG_REAL + 10)        // 10
#define REG_RL_BCHRG_Ushunt       (REG_RL_BATTERY + 0)
#define REG_RL_BCHRG_Ubus         (REG_RL_BATTERY + 1)
#define REG_RL_BCHRG_Pbus         (REG_RL_BATTERY + 2)
#define REG_RL_BCHRG_Ibus         (REG_RL_BATTERY + 3)
#define REG_RL_BCHRG_Ibus_Max     (REG_RL_BATTERY + 4)

#define REG_RL_BTOUT_Ushunt       (REG_RL_BATTERY + 5)
#define REG_RL_BTOUT_Ubus         (REG_RL_BATTERY + 6)
#define REG_RL_BTOUT_Pbus         (REG_RL_BATTERY + 7)
#define REG_RL_BTOUT_Ibus         (REG_RL_BATTERY + 8)
#define REG_RL_BTOUT_Ibus_Max     (REG_RL_BATTERY + 9)

#define REG_RL_BAT_CAPLevel       (REG_RL_BATTERY + 10)  // 按%标定的电池电量
#define REG_RL_BAT_TEMPERATRUE    (REG_RL_BATTERY + 11)

// 逆变器调整输出电压的中间过程
#define REG_RL_COIL               (REG_REAL + 23)         // 25
#define REG_RL_COIL_Volt          (REG_RL_COIL + 0)

// 通用测量寄存器
// 测量结果
#define  REG_RL_MEASURE           (REG_REAL + 27)         // 27
#define  REG_RL_Uin               (REG_RL_MEASURE + 0)
#define  REG_RL_Uout              (REG_RL_MEASURE + 1)

//--------------------------
// 保护数据
#define  REG_RL_PROECT            (REG_RL_MEASURE + 2)    // 30

// 保护测量
#define  REG_RL_UinVal            (REG_RL_PROECT + 0)
#define  REG_RL_UoutVal           (REG_RL_PROECT + 1)

//=============================================================================
// 事件寄存器                     
//-----------------------------------------------------------------------------
// 装置硬件事件
#define REG_EVT_HWINSPECT         (REG_EVENT + 0)
#define REG_EH_POWEROFF           (REG_EVT_HWINSPECT + 0)
#define REG_EH_POWERON            (REG_EVT_HWINSPECT + 1)
#define REG_EH_POWERON_PWR        (REG_EVT_HWINSPECT + 2)
#define REG_EH_POWERON_WDG        (REG_EVT_HWINSPECT + 3)
#define REG_EH_POWERON_SFT        (REG_EVT_HWINSPECT + 4)
#define REG_EH_WDG_ACTIVE         (REG_EVT_HWINSPECT + 8)
#define REG_EH_HSE_FAULT          (REG_EVT_HWINSPECT + 9)
#define REG_EH_LSE_FAULT          (REG_EVT_HWINSPECT + 10)
#define REG_EH_RCC_FAULT          (REG_EVT_HWINSPECT + 11)
#define REG_EH_BAT_FAULT          (REG_EVT_HWINSPECT + 12)
#define REG_EH_RTC_FAULT          (REG_EVT_HWINSPECT + 13)
#define REG_EH_FeRAM_FAULT        (REG_EVT_HWINSPECT + 14)
#define REG_EH_ExFLASH_FAULT      (REG_EVT_HWINSPECT + 15)
#define REG_EH_ADC_FAULT          (REG_EVT_HWINSPECT + 16)
#define REG_EH_EXADC1_FAULT       (REG_EVT_HWINSPECT + 17)
#define REG_EH_EXADC2_FAULT       (REG_EVT_HWINSPECT + 18)
#define REG_EH_ExRTC_FAULT        (REG_EVT_HWINSPECT + 19)
#define REG_EH_BATLOW_FAULT       (REG_EVT_HWINSPECT + 20)
#define RHF_EH_RHEOSTAT_FAULT     (REG_EVT_HWINSPECT + 21)

// 配置数据相关事件
#define REG_EVT_CONFIG            (REG_EVENT + 30)
#define REG_EC_DEVCFGERR          (REG_EVT_CONFIG + 0)    // 设备参数错误
#define REG_EC_DEVCFGINIT         (REG_EVT_CONFIG + 1)    // 设备参数初始化
#define REG_EC_HOLDINGERR         (REG_EVT_CONFIG + 2)    // 保护压板定值错误
#define REG_EC_HOLDINGINIT        (REG_EVT_CONFIG + 3)    // 保护压板定值错误
#define REG_EC_EVENTLOGERR        (REG_EVT_CONFIG + 4)    // 报告记录错误
#define REG_EC_EVENTLOGINIT       (REG_EVT_CONFIG + 5)    // 报告记录初始化
#define REG_EC_HOLDINGFAULT       (REG_EVT_CONFIG + 6)    // 保护压板定值失效
#define REG_EC_HOLDINGRECOVER     (REG_EVT_CONFIG + 7)    // 保护压板定值修复

// 装置操作事件
#define REG_EVT_OPERATE           (REG_EVENT + 40)
#define REG_EO_SET_RTC            (REG_EVT_OPERATE + 0)   // 设置时间
#define REG_EO_CLR_EVENT          (REG_EVT_OPERATE + 1)   // 清【事件记录】
#define REG_EO_CLR_ALARM          (REG_EVT_OPERATE + 2)   // 清【报警记录】
#define REG_EO_CLR_FAULT          (REG_EVT_OPERATE + 3)   // 清【事故记录】
#define REG_EO_CLR_WAVELOG        (REG_EVT_OPERATE + 4)   // 清【波形记录】
#define REG_EO_DEF_HOLDING        (REG_EVT_OPERATE + 5)   // 读默认定值
#define REG_EO_DEF_DEVCFG         (REG_EVT_OPERATE + 6)   // 读默认设备参数
#define REG_EO_CHGACTHOLD         (REG_EVT_OPERATE + 7)   // 切换运行定值区
#define REG_EO_COPYHOLDING        (REG_EVT_OPERATE + 8)   // 复制定值区
#define REG_EO_SETDEVCFG          (REG_EVT_OPERATE + 9)   // 修改装置参数
#define REG_EO_SETHLDSWT          (REG_EVT_OPERATE +10)   // 修改压板
#define REG_EO_SETHLDATA          (REG_EVT_OPERATE +11)   // 修改定值
#define REG_EO_SETNETCFG          (REG_EVT_OPERATE +12)   // 修改通信配置
#define REG_EO_SETCTRLMX          (REG_EVT_OPERATE +13)   // 修改控制矩阵
#define REG_EO_SETPASSWORD        (REG_EVT_OPERATE +14)   // 修改操作口令
#define REG_EO_SETPASSWORDx       (REG_EVT_OPERATE +15)   // 修改超级口令
#define REG_EO_SETCALIBRATION     (REG_EVT_OPERATE +16)   // 修改校准系数
//#define REG_EO_SETDEVFUNCTYPE     (REG_EVT_OPERATE +17)   // 修改功能类型
//#define REG_EO_SETDISTRIBUTOR     (REG_EVT_OPERATE +18)   // 修改经销商
//#define REG_EO_SIGNALS_RETURN     (REG_EVT_OPERATE +19)   // 信号复归
//#define REG_EO_WAVELOG_TRIGGER    (REG_EVT_OPERATE +20)   // 触发录波

//#define REG_EO_BREAK_ON_OPEATE    (REG_EVT_OPERATE +30)   // 手动合闸操作
//#define REG_EO_BREAK_OFF_OPEATE   (REG_EVT_OPERATE +31)   // 手动分闸操作

// 远程操作事件 
#define REG_EVT_COMM              (REG_EVT_OPERATE +80)
#define REG_EM_SET_RTC            (REG_EVT_COMM + 0)      // 设置时间
#define REG_EM_CONFIG             (REG_EVT_COMM + 1)      // 配置
//#define REG_ER_SGLRET             (REG_EVT_COMM + 2)      // 信号复归

// 遥控操作
#define REG_EVT_RMTCTRL           (REG_EVT_COMM + 10)      
#define REG_ER_RELAY0             (REG_EVT_RMTCTRL + 0)   // 遥控0
#define REG_ER_RELAY1             (REG_EVT_RMTCTRL + 1)   // 遥控1

//=============================================================================
// 功能寄存器
//-----------------------------------------------------------------------------
// [设备配置]编辑
#define REG_FN_DEV_OPTION         (REG_FUNCTION +  0)
#define REG_FN_PROTECT_EN         (REG_FN_DEV_OPTION + 0) // 保护总压板
//#define REG_FN_SOE_AUTORET        (REG_FN_DEV_OPTION + 1) // SOE自动复归
//#define REG_FN_CT_1A              (REG_FN_DEV_OPTION + 2) // 1A CT
//#define REG_FN_VOLTAGE_MODE       (REG_FN_DEV_OPTION + 3) // 电压接线方式

// [设备参数]编辑
#define REG_FN_DEV_CONFIG         (REG_FN_DEV_OPTION + 10)                 // 10
#define REG_DEVFUNC               (REG_FN_DEV_CONFIG + 0) // 装置功能类型       
#define REG_FN_DEVFUNC            (REG_FN_DEV_CONFIG + 1) // 装置功能类型(用于编辑)

#define REG_ACTV_HOLD_BLK         (REG_FN_DEV_CONFIG + 2) // 当前运行定值区
#define REG_FN_ACTV_HOLD_BLK      (REG_FN_DEV_CONFIG + 3) // 当前运行定值区(用于编辑)

#define REG_EDIT_HOLD_BLK         (REG_FN_DEV_CONFIG + 4) // 当前编辑定值区
#define REG_FN_EDIT_HOLD_BLK      (REG_FN_DEV_CONFIG + 5) // 当前编辑定值区(用于编辑)

#define REG_FN_COPY_HOLD_BLK      (REG_FN_DEV_CONFIG + 6) // 将当前编辑的定值区复制到指定区

//#define REG_DISTRIBUTOR           (REG_FN_DEV_CONFIG + 7) // 产品供应商
//#define REG_FN_Distributor        (REG_FN_DEV_CONFIG + 8) // 产品供应商(用于编辑)

#define REG_LANGUAGE              (REG_FN_DEV_CONFIG + 9)  // 界面语言
#define REG_FN_Language           (REG_FN_DEV_CONFIG + 10) // 界面语言(用于编辑)

#define REG_PASSWORD              (REG_FN_DEV_CONFIG + 11) // 操作口令(写操作用于检验口令，结果在REG_Password1Ok)
#define REG_FN_PASSWORD           (REG_FN_DEV_CONFIG + 12) // 操作口令(用于编辑)

#define REG_PASSWORD2             (REG_FN_DEV_CONFIG + 13) // 配置口令
#define REG_FN_PASSWORD2          (REG_FN_DEV_CONFIG + 14) // 配置口令(用于编辑)

#define REG_PASSWORDx             (REG_FN_DEV_CONFIG + 15) // 动态口令

// 
#define REG_FN_DEV_ADDR           (REG_FN_DEV_CONFIG + 20) // 装置地址
#define REG_FN_WAVELOG_PRE        (REG_FN_DEV_CONFIG + 21) // 录波器预录周期

#define REG_FN_AUTOCTRL_EN        (REG_FN_DEV_CONFIG + 24) // 允许自动控制
#define REG_FN_AUTO_TURNOFF       (REG_FN_DEV_CONFIG + 25) // 自动关机
#define REG_FN_AUTO_BREAKER_ON    (REG_FN_DEV_CONFIG + 26) // 允许自动合闸
#define REG_FN_PASSBY_EN          (REG_FN_DEV_CONFIG + 27) // 允许旁路运行

#define REG_FN_ACTION_VOLTAGE     (REG_FN_DEV_CONFIG + 30) // 线圈投入的电压阀值%
#define REG_FN_COIL_VOLTAGE       (REG_FN_DEV_CONFIG + 31) // 线圈供电电压
#define REG_FN_PWRON_TIME         (REG_FN_DEV_CONFIG + 32) // 得电待持时间s
#define REG_FN_PWROFF_TIME        (REG_FN_DEV_CONFIG + 33) // 失电持续时间s
#define REG_FN_SHUTDOWN_TIME      (REG_FN_DEV_CONFIG + 34) // 自动关机时间s
#define REG_FN_RELAY_DELAY        (REG_FN_DEV_CONFIG + 35) // CK1继电器启动延时s
#define REG_FN_RELAY_TIME         (REG_FN_DEV_CONFIG + 36) // CK1继电器动作时间s

#define REG_FN_DATETIME           (REG_FN_DEV_CONFIG + 40) // 同时访问日期时间       40
#define REG_DATE_YEAR             (REG_FN_DATETIME + 1)    // 年
#define REG_DATE_MONTH            (REG_FN_DATETIME + 2)    // 月
#define REG_DATE_DAY              (REG_FN_DATETIME + 3)    // 日
#define REG_TIME_HOUR             (REG_FN_DATETIME + 4)    // 时
#define REG_TIME_MINUTE           (REG_FN_DATETIME + 5)    // 分
#define REG_TIME_SECOND           (REG_FN_DATETIME + 6)    // 秒
#define REG_TIME_MSECOND          (REG_FN_DATETIME + 7)    // 毫秒

// 串口1配置
#define REG_UART1_CFG             (REG_FN_DEV_CONFIG + 50)    // 可读回TUARTConfig指针 210
#define REG_UART1_ADDR            (REG_UART1_CFG + 1)       //
#define REG_UART1_BAUDRATE        (REG_UART1_CFG + 2)       //
#define REG_UART1_PARITY          (REG_UART1_CFG + 3)       //
#define REG_UART1_STOPBITS        (REG_UART1_CFG + 4)       //
#define REG_UART1_PROTOCOL        (REG_UART1_CFG + 5)       //

// 频谱分析相关
// 取频谱数据区
#define GetACSpectrum(n)          ((TACSpectrum*)&DevCache.ACSpectrum[n])

#define REG_FUNCTION_USER         (REG_FUNCTION + 400)    // 用户可使用的功能寄存器
//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 实始化寄存器
// 输入：无
// 输出：无
// 返回：无
void DevReg_Init(void);
//-----------------------------------------------------------------------------
// 寄存器访问
//-----------------------------------------------------------------------------
// 读取寄存器
// 输入：
//    uReg:寄存器号
// 输出：无
// 返回：寄存器值
uint32_t DevReg_Read ( uint32_t uReg );
//-----------------------------------------------------------------------------
// 写寄存器
// 输入：
//    uReg: 寄存器号
//  uValue: 要写入寄存器的值
// 输出：无
// 返回：无
void DevReg_Write ( uint32_t uReg, uint32_t uValue );
//-----------------------------------------------------------------------------
// 取指定类别的寄存器数量
// 输入：
//     uReg: 寄存器号
// 输出：无
// 返回：
//   寄存器数量
uint32_t DevReg_Count ( uint32_t uReg );
//=============================================================================
// 寄存器的快速访问方法
// 在调试状态( __DEBUG )下使用函数访问，否则用直接寄存器访问
//-----------------------------------------------------------------------------
#undef  __REGDEBUG
#ifndef __REGDEBUG
  // 取设置配置寄存器
  #define  _GetDevCfgReg(reg)         (DevConfig.Configs[(reg) - REG_DEVCONFIG])

  // 取压板寄存器
  #define  _GetSwitchReg(reg)         (DevHolding.Data.Switch[(reg) - REG_SWITCH])

  // 取定值寄存器
  #define  _GetHoldingReg(reg)        (DevHolding.Data.Data[(reg) - REG_HOLDING])

  // 取IO状态寄存器 
  #define  _GetIOStateReg(reg)        (DevCache.IOState[(reg) - REG_IOSTATE])
  // 写IO状态寄存器
  #define  _SetIOStateReg(reg, val)   (DevCache.IOState[(reg) - REG_IOSTATE] = val)

  // 取状态寄存器
  #define  _GetStateReg(reg)          (DevCache.State[(reg) - REG_STATE])
  // 写状态寄存器 
  #define  _SetStateReg(reg, val)     (DevCache.State[(reg) - REG_STATE] = val)
  // 取通用寄存器
  #define  _GetCommonReg(reg)         (DevCache.Common[(reg) - REG_COMMON])
  // 设置通用寄存器
  #define  _SetCommonReg(reg, val)    (DevCache.Common[(reg) - REG_COMMON])
  // 取浮点寄存器
  #define  _GetRealReg(reg)           (DevCache.Reals[(reg) - REG_REAL])
  // 设置浮点寄存器 
  #define  _SetRealReg(reg, val)      (DevCache.Reals[(reg) - REG_REAL] = val)
#else
  // 取设置配置寄存器
  TDevStateReg  _GetDevCfgReg(uint32_t reg);
   
  // 取压板寄存器
  TStateReg     _GetSwitchReg(uint32_t reg);     
  
  // 取定值寄存器
  uint32_t  _GetHoldingReg(uint32_t reg);
  
  // 取IO状态
  TStateReg  _GetIOStateReg(uint32_t reg); 

  // 取状态寄存器
  TStateReg  _GetStateReg(uint32_t reg); 
  // 取状态寄存器
  TStateReg  __GetStateReg_bk(uint32_t reg); 
  // 写状态寄存器
  void       _SetStateReg(uint32_t reg, TStateReg val); 
  // 写状态寄存器备份区
  void       __SetStateReg_bk(uint32_t reg, TStateReg val);   
  // 取通用寄存器
  TCommonReg _GetCommonReg(uint32_t reg);
  // 设置通用寄存器
  void       _SetCommonReg(uint32_t reg, TCommonReg val);
  // 取浮点寄存器
  TRealReg    _GetRealReg(uint32_t reg);
  // 设置浮点寄存器
  void       _SetRealReg(uint32_t reg, TRealReg val);
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
