//-----------------------------------------------------------------------------
/*
 File        : rtc.c
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f4xx
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description :STM32F4内部RTC封装函数
        
 Date       : 2023.12.05 2017.08.12
*/
//-----------------------------------------------------------------------------
#include "rtc.h"

#include "SD3077.h"
#include "DevRegs.h"
#include "DevClock.h"
#include "DevEvtMgr.h"

#include <stm32f4xx_hal.h>

#include <cmsis_os.h>
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
// 预分频
#define PERDIV_A           0x1F
#define PERDIV_S           0x3FF
//-----------------------------------------------------------------------------
// Back寄存器分配
//-----------------------------------------------------------------------------
// RTC配置标记
#define BKGREG_RTCCFG      RTC->BKP0R

#define RTC_CONFIGED       0xAC6EE635
//-----------------------------------------------------------------------------
// 控制器动作的启动时刻
#define NUM_CONTROLLOR_STATES   8           // 控制器状态数据数量
#define CONTROLLOR_STATE_INDEX  (0x50 + 2 * sizeof(uint32_t))  // 保存控制器状态数据的起始地址
//-----------------------------------------------------------------------------
// 持续状态
#define BKGREG_PERSIS1     RTC->BKP15R 
#define BKGREG_PERSIS2     RTC->BKP16R 
//-----------------------------------------------------------------------------
// IAP有效标记
#define BKGREG_IAP1        RTC->BKP17R
#define BKGREG_IAP2        RTC->BKP18R
#define BKGREG_IAP3        RTC->BKP19R

#define IAP_ENABLED_TOKEN  (uint32_t)0xC8D9C6BD
#define IAP_ENABLED_CHECK  (uint32_t)(IAP_ENABLED_TOKEN ^ IAP_ENABLED_SIGN)
//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------
RTC_HandleTypeDef hrtc;

//static uint32_t   FuMillisecond;
//=============================================================================
// 局部方法
//-----------------------------------------------------------------------------
// HAL_RCC_OscConfig中LSI启动时间较长，造成看门狗启动 
HAL_StatusTypeDef RTC_LSI_OscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
  
  uint32_t tickstart = 0;  
  
  /*------------------------------ LSI Configuration -------------------------*/
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI)
    {
    /* Check the parameters */
    assert_param(IS_RCC_LSI(RCC_OscInitStruct->LSIState));
    
    /* Check the LSI State */
    if((RCC_OscInitStruct->LSIState)!= RCC_LSI_OFF)
      {
      /* Enable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_ENABLE();
      
      /* Get Start Tick*/
      tickstart = HAL_GetTick();
      
      /* Wait till LSI is ready */
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET)
        {
        if((HAL_GetTick() - tickstart ) > LSI_TIMEOUT_VALUE)
          {
          SetHWFault( RHF_LSI_ERR );

          return HAL_TIMEOUT;
          } 
        }
      }
    else
      {
      /* Disable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_DISABLE();
      
      /* Get Start Tick*/
      tickstart = HAL_GetTick();
      
      /* Wait till LSI is ready */  
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) != RESET)
        {
        if((HAL_GetTick() - tickstart ) > LSI_TIMEOUT_VALUE)
          {
          SetHWFault( RHF_LSI_ERR );

          return HAL_TIMEOUT;
          }       
        } 
      }
    }
  
  return HAL_OK;
}
//-----------------------------------------------------------------------------
// Set DateTime
static HAL_StatusTypeDef _SetDateTime( const TDateTimeType *sTime)
{

  int iRes;
  
  RTC_DateTypeDef  Date;
  RTC_TimeTypeDef  Time;
  
//  // 发送“修改系统时钟”事件
//  EVTMGR_AppendEvent( REG_EO_SET_RTC, STATE_TRUE );

  assert_param( sTime );

  __HAL_RCC_PWR_CLK_ENABLE();  // PWREN <- 1
  HAL_PWR_EnableBkUpAccess();  // DBP <- 1

  Date.Year    = sTime->Year - YEAR_BEGIN;
  Date.Month   = sTime->Month;
  Date.Date    = sTime->Day;
  Date.WeekDay = sTime->WeekDay;
  
  Time.Hours      = sTime->Hours;
  Time.Minutes    = sTime->Minutes;
  Time.Seconds    = sTime->Seconds;
  Time.SubSeconds = sTime->MilSeconds;
//  FuMillisecond   = sTime->MilSeconds;
  Time.TimeFormat = (sTime->Hours < 12) ? RTC_HOURFORMAT12_AM : RTC_HOURFORMAT12_PM;
  Time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  Time.StoreOperation = RTC_STOREOPERATION_RESET;
  
  HAL_PWR_EnableBkUpAccess();

  if( HAL_OK == HAL_RTC_SetTime( &hrtc,  &Time, RTC_FORMAT_BIN ) &&
      HAL_OK == HAL_RTC_SetDate( &hrtc,  &Date, RTC_FORMAT_BIN ) )
    {
    iRes = HAL_OK;

    ClrHWFault( RHF_RTC_ERR );
    }
  else
    {
    iRes = HAL_ERROR;

    SetHWFault( RHF_RTC_ERR );

    EVTMGR_AppendEvent( REG_EH_RTC_FAULT, EVENT_TRUE );
    }
  
  HAL_PWR_DisableBkUpAccess();

  HAL_RTCEx_DeactivateCalibrationOutPut( &hrtc );
  HAL_RTCEx_DeactivateTamper( &hrtc, RTC_TAFCR_TAMP1E | RTC_TAFCR_TAMP2E );
  
  return iRes;
}
//-----------------------------------------------------------------------------
// Get DateTime from MCU-RTC
static HAL_StatusTypeDef _GetDateTime( TDateTimeType *sTime)
{

  int iRes;
  
  RTC_DateTypeDef  Date;
  RTC_TimeTypeDef  Time;

  assert_param( sTime );
  
  if( 0 == GetHWFault( RHF_RTC_ERR ) &&
      HAL_OK == HAL_RTC_GetTime( &hrtc,  &Time, RTC_FORMAT_BIN ) &&
      HAL_OK == HAL_RTC_GetDate( &hrtc,  &Date, RTC_FORMAT_BIN ) )
    {
//    if( 1 < Time.SecondFraction )
//      FuMillisecond = 1000 - (Time.SubSeconds * 1000u) / Time.SecondFraction;
      
    sTime->Year    = Date.Year + YEAR_BEGIN;
    sTime->Month   = Date.Month;
    sTime->Day     = Date.Date;
    sTime->WeekDay = Date.WeekDay;
      
    sTime->Hours   = Time.Hours;
    sTime->Minutes = Time.Minutes;
    sTime->Seconds = Time.Seconds;
    sTime->MilSeconds = DevClock.FMilSecs % 1000; //FuMillisecond;
    
    iRes = HAL_OK;
    }
  else
    iRes = HAL_ERROR;
  
  return iRes;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// RTC init function
void MX_RTC_Init(void)
{

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  uint32_t u3077Ok = SD3077_Init();
  if( 0 == u3077Ok )
    {
    SetHWFault( RHF_ExRTC_ERR );
    }
  else
    osDelay( 10 );
  
  __HAL_RCC_PWR_CLK_ENABLE();  // PWREN <- 1
  HAL_PWR_EnableBkUpAccess();  // DBP <- 1
  // Enable BKPSRAM Clock
  __HAL_RCC_BKPSRAM_CLK_ENABLE();

  if( BKGREG_RTCCFG != RTC_CONFIGED )
    {      
    RCC_PeriphCLKInitTypeDef  PeriphClkInit;
    HAL_RCCEx_GetPeriphCLKConfig( &PeriphClkInit );
  
    int bConfiged = 0;
    if( RCC_RTCCLKSOURCE_LSE != PeriphClkInit.RTCClockSelection )
      {
      int iTryCntr = 2;
      while( iTryCntr-- > 0 )
        {
        // 读各晶振状态
        RCC_OscInitTypeDef     RCC_OscInitStruct;
        HAL_RCC_GetOscConfig( &RCC_OscInitStruct );
  
        // LSE有效？
        if( RCC_LSE_ON == RCC_OscInitStruct.LSEState )
          {
          // LSE有效，用LSE
          RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
          PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
          PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
          HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
          
          HAL_RCCEx_GetPeriphCLKConfig( &PeriphClkInit );
            
          if( RCC_RTCCLKSOURCE_LSE == PeriphClkInit.RTCClockSelection )
            {
            bConfiged = 0xAA;
            break;
            }
          }
        else
          {
          // 启动LSE
          RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
          RCC_OscInitStruct.LSEState = RCC_LSE_ON;
          if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
            {
            SetHWFault( RHF_LSE_ERR );
            }
          else
            {
            ClrHWFault( RHF_LSE_ERR );
            iTryCntr++;
            }
            
          // 即使LSE启动成功，也需要回去配置RTC的时钟源为LSE
          if( iTryCntr > 1 )
            continue;

//          // 启动LSE失败
//          // 尝试启动LSI
//          RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
//          RCC_OscInitStruct.LSIState       = RCC_LSI_ON;

//          if (RTC_LSI_OscConfig(&RCC_OscInitStruct) == HAL_OK )
//            {
//            RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
//            PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
//            PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;
//            HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
//            
//            HAL_RCCEx_GetPeriphCLKConfig( &PeriphClkInit );
//              
//            if( RCC_RTCCLKSOURCE_LSI == PeriphClkInit.RTCClockSelection )
//              {
//              bConfiged = 0xAA;
//              break;
//              }

//            ClrHWFault( RHF_LSI_ERR );
//            }
//          else
//            {
//            SetHWFault( RHF_LSI_ERR );
//            }
          }
        }  // while
      }
    else
      bConfiged = 0xAA;

    if( 0xAA == bConfiged )
      // 置“RTC已启动”标志
      BKGREG_RTCCFG = RTC_CONFIGED;
    }
  
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
  hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.AsynchPrediv   = 0x7F;
  hrtc.Init.SynchPrediv    = 0xF9; // 32K / (AsynchPrediv - 1) - 1
  if( HAL_RTC_Init( &hrtc ) != HAL_OK )
    {
    SetHWFault( RHF_RTC_ERR );

    EVTMGR_AppendEvent( REG_EH_RTC_FAULT, EVENT_TRUE );
    }

  // 更新 ms计数
//  FuMillisecond = RTC->SSR;

  // 禁止AF1，释放PC13
  HAL_RTCEx_DeactivateCalibrationOutPut( &hrtc );
  HAL_RTCEx_DeactivateTamper( &hrtc, RTC_TAFCR_TAMP1E | RTC_TAFCR_TAMP2E );
    
  //---------- 初始化外部RTC
  if( 0 < u3077Ok )
    {
    // 外部RTC可访问
    uint32_t uSt3077;
    int iRes = SD3077_ReadState( &uSt3077 );
    if( 0 == iRes )
      {
#ifdef SetRTCStReg
      SetRTCStReg( uSt3077 ); // 保存ExRTC的状态
#endif
      if( 0 != (uSt3077 & (SD3077_STATE_RTCF | SD3077_STATE_OSF) ) )
        {
        // 外部RTC的时间不可信
        TDateTimeType dtTime;
        if( HAL_OK == _GetDateTime( &dtTime ) )
          {
          // 用内部时间校外部时钟
          if( 0 == SD3077_SetDateTime( &dtTime ) )
            ClrHWFault( RHF_ExRTC_ERR );
          }
        }
      else
        {
        TDateTimeType dtTime;
        if( 0 == SD3077_GetDateTime( &dtTime ) )
          {
          _SetDateTime( &dtTime );
          
          DEVCLK_SetDateTime( &DevClock, &dtTime );
            
          ClrHWFault( RHF_ExRTC_ERR );
          }
        else if( HAL_OK == _GetDateTime( &dtTime ) )
          {
          // 用内部时间校外部时钟
          if( 0 == SD3077_SetDateTime( &dtTime ) )
            ClrHWFault( RHF_ExRTC_ERR );
          }
        }

      if( 0 != (uSt3077 & EXRTC_BLF) )
        EVTMGR_AppendEvent( REG_EH_BAT_FAULT, EVENT_TRUE );
      }
    else
      {
      SetHWFault( RHF_ExRTC_ERR );

      EVTMGR_AppendEvent( REG_EH_ExRTC_FAULT, EVENT_TRUE );
      }
    }
}
//-----------------------------------------------------------------------------
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  __HAL_RCC_RTC_ENABLE();
}
//-----------------------------------------------------------------------------
//void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
//{

//  if(rtcHandle->Instance == RTC)
//    {
//    /* USER CODE BEGIN RTC_MspDeInit 0 */
//    
//    /* USER CODE END RTC_MspDeInit 0 */
//      /* Peripheral clock disable */
//      __HAL_RCC_RTC_DISABLE();
//    /* USER CODE BEGIN RTC_MspDeInit 1 */
//    
//    /* USER CODE END RTC_MspDeInit 1 */
//    }
//} 
//-----------------------------------------------------------------------------
// Set DateTime
int RTC_SetTime( const TDateTimeType *sTime)
{

  // 设置内部RTC
  int iRes = HAL_OK;
  if( HAL_OK != _SetDateTime( sTime ) )
    iRes = HAL_ERROR;
  
  if( 0 != SD3077_SetDateTime( sTime ) )
    iRes = HAL_ERROR;

  DEVCLK_SetDateTime( &DevClock, sTime );

  return iRes;
}
//-----------------------------------------------------------------------------
// Get DateTime
int RTC_GetTime( TDateTimeType *sTime)
{
  
  int iRes;
  
  if( HAL_OK == _GetDateTime(sTime) )
    {
    iRes = HAL_OK;
    }
  else if( 0 == GetHWFault(RHF_ExRTC_ERR) )
    {
    if( 0 == SD3077_GetDateTime( sTime ) )
      {
      sTime->MilSeconds = DevClock.FMilSecs % 1000;

      iRes = HAL_OK;
      }
    else
      iRes = HAL_ERROR;
    }
  else
    {
    DEVCLK_GetDateTime( &DevClock, sTime );
      
    iRes = HAL_OK;
    }
  
  return iRes;
}
//-----------------------------------------------------------------------------
// 用ExRTC同步片内时间
int RTC_ExRTCSync()
{

  int iRes = HAL_OK;
  TDateTimeType sTime;

  if( 0 == SD3077_GetDateTime( &sTime ) )
    {
    if( HAL_OK != _SetDateTime( &sTime ) )
      iRes = HAL_ERROR;

    DEVCLK_SetDateTime( &DevClock, &sTime );
    }
  else
    iRes = HAL_ERROR;

  return iRes;
}
//-----------------------------------------------------------------------------
// 填写事件时间
void RTC_FillEventTime( TEventLogSummary *pEvtLog )
{

  TDateTimeType dsTime;
  if( HAL_OK == RTC_GetTime( &dsTime ) )
    {
    pEvtLog->Time.Year    = dsTime.Year;
    pEvtLog->Time.Month   = dsTime.Month;
    pEvtLog->Time.Day     = dsTime.Day;
      
    pEvtLog->Time.Hours   = dsTime.Hours;
    pEvtLog->Time.Minutes = dsTime.Minutes;
    pEvtLog->Time.Seconds = dsTime.Seconds;
    pEvtLog->Time.milSecs = dsTime.MilSeconds;
    }
}
//-----------------------------------------------------------------------------
//// 毫秒计数
//void RTC_msTick(void)
//{

//  FuMillisecond++;
//  if( FuMillisecond > 999 )
//    FuMillisecond = 0;
//}
//------------------------------------------------------------------------------
// 取系统毫秒Tick
uint32_t RTC_GetSysMsTick(void)
{
  
  return HAL_GetTick();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// IAP相关
//------------------------------------------------------------------------------
// 读IAP标识
uint32_t RTC_ReadIAPIDReg(void)
{

  uint32_t uSign, uToken, uChkSv;
  
  uSign  = BKGREG_IAP1,
  uToken = BKGREG_IAP2,
  uChkSv = BKGREG_IAP3;

  if( IAP_ENABLED_SIGN  == uSign  &&
      IAP_ENABLED_TOKEN == uToken &&
      IAP_ENABLED_CHECK == uChkSv )
    return IAP_ENABLED_SIGN;
  
  return 0;  
}
//------------------------------------------------------------------------------
// 写IAP标识
uint32_t RTC_MarkIAPIDReg(uint32_t uToken)
{

  __HAL_RCC_PWR_CLK_ENABLE();  // PWREN <- 1
  HAL_PWR_EnableBkUpAccess();  // DBP <- 1

  if( IAP_ENABLED_SIGN == uToken )
    {
    BKGREG_IAP1 = IAP_ENABLED_SIGN;
    BKGREG_IAP2 = IAP_ENABLED_TOKEN;
    BKGREG_IAP3 = IAP_ENABLED_CHECK;
    }
  else
    {
    BKGREG_IAP1 = 0;
    BKGREG_IAP2 = 0;
    BKGREG_IAP3 = 0;
    }
  
  HAL_PWR_DisableBkUpAccess();

  return RTC_ReadIAPIDReg();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 控制器状态保存
//-----------------------------------------------------------------------------
// 读回保存的控制器状态
// 输入:
//   uIndex：数据的位置索引
// 返回：
//   呆存的状态数据值
uint32_t RTC_ReadCtrlState(uint32_t uIndex)
{
  
  if( NUM_CONTROLLOR_STATES <= uIndex )
    return 0;

  uint32_t uRegOffset = CONTROLLOR_STATE_INDEX + uIndex * sizeof(uint32_t);
  uint32_t uRes = (*(__IO uint32_t*)(RTC_BASE + uRegOffset));
  
  return uRes;
}
//-----------------------------------------------------------------------------
// 写入控制器状态数据
// 输入:
//   uIndex：数据的位置索引
// 返回：
//   0: 成功  else 失败
int RTC_WriteCtrlState(uint32_t uIndex, uint32_t uData)
{

  if( NUM_CONTROLLOR_STATES <= uIndex )
    return -1;

  __HAL_RCC_PWR_CLK_ENABLE();  // PWREN <- 1
  HAL_PWR_EnableBkUpAccess();  // DBP <- 1

  uint32_t uRegOffset = CONTROLLOR_STATE_INDEX + uIndex * sizeof(uint32_t);
  *((__IO uint32_t*)(RTC_BASE + uRegOffset)) = uData;
  
  HAL_PWR_DisableBkUpAccess();
  
  return RTC_ReadCtrlState(uIndex);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  非易失相关
//------------------------------------------------------------------------------
// 读BackReg
uint32_t RTC_ReadBKReg(uint32_t uReg)
{

  uint32_t uResult;
  switch( uReg )
    {
    case 0:
      uResult = RTC->BKP10R;
      break;
    default:
      uResult = 0;
      break;
    }
  return uResult;
}
//------------------------------------------------------------------------------
// 写BackReg
void RTC_WriteBkReg(uint32_t uReg, uint32_t uValue)
{

  __HAL_RCC_PWR_CLK_ENABLE();  // PWREN <- 1
  HAL_PWR_EnableBkUpAccess();  // DBP <- 1

  switch( uReg )
    {
    case 0:
      RTC->BKP10R = uValue;
      break;
    default:
      break;
    }
  
  HAL_PWR_DisableBkUpAccess();
}
//------------------------------------------------------------------------------
