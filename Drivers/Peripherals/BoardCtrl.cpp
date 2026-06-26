//-----------------------------------------------------------------------------
/*
 File        : BoardCtrl.c
 Version     : V1.10
 By          : 银网科技

 Description : 功率板控制接口
        
 Date        : 2023.12.05

 2024/10/5
 Ver 2.1  Wey
   适配V2.1硬件
*/
//-----------------------------------------------------------------------------
#include "BoardCtrl.h"

#include "gpio.h"

#include "MCP401x.h"

#include "DevRegs.h"
#include "DevEvtMgr.h"

#include "DevDebug.h"

#include <string.h>
//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------
// 目标电压
// 存放的是
static uint8_t  FucCVTarget = 0, FucCVCurrent;

//=============================================================================
// 局部方法
//-----------------------------------------------------------------------------

//=============================================================================
// 局部方法实现
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
//// 调节逆变器电压
//void BoardCtrl_PowersupplyAdjust(uint32_t uToken)
//{
//  
//  if( TOKEN_BRDSET_ISCORRECT(uToken) )
//    {
//    uint32_t uVoltage = TOKEN_BRDSET_GET( uToken );
//    
//    float fVR = (150.0 * 1.25) / (uVoltage - 1.25) - 2.72;

//    uint32_t uWiper;
//    if( fVR <= 0 )
//      fVR = 0.0f;
//    else if( fVR > 5.0f )
//      fVR = 5.0f;
//    else
//      uWiper = fVR * 127.0 / 5.0 + 0.5;
//    
//    MCP401X_SetWiper( TOKEN_BRDSET_SET(uWiper) );
//    }
//}
////-----------------------------------------------------------------------------
//static int _SetWiper(uint32_t uToken)
//{
//  
//  int iRes;
//  if( TOKEN_BRDSET_ISCORRECT(uToken) )
//    {
//    uint32_t uWiper = TOKEN_BRDSET_GET( uToken );
//      
//    iRes = MCP401X_SetWiper( uWiper );
//    }
//  else
//    iRes = -101;
//  
//  return iRes;
//}
//=============================================================================
// 全局方法实现
//-----------------------------------------------------------------------------
// 初始化
void BoardCtrl_Init(void)
{
  
  if( 0 == MCP401X_Init() )
    {
    SetHWFault( RHF_RHEOSTAT_ERR );

    EVTMGR_AppendEvent( RHF_EH_RHEOSTAT_FAULT, EVENT_TRUE );
    }
  else
    {
//    // 按配置数据设置输出电压
//    uint32_t uVoltage = _GetDevCfgReg(REG_COIL_VOLTAGE);
//    if( INVERTORVOL_MIN <= uVoltage && INVERTORVOL_MAX >= uVoltage )
//      BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(uVoltage) );

    BoardCtrl_SetInvertorVoltage( TOKEN_BRDSET_SET(INVERTER_VOLTAGE) );

    uint32_t uWiper;
    if( 0 == MCP401X_ReadWiper( &uWiper ) )
      FucCVCurrent = uWiper;
    else
      FucCVCurrent = 0;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 定时激励
// 调整逆变器电压时，采用逐步提升的方式
void BoardCtrl_Tick(void)
{

  if( FucCVTarget != FucCVCurrent )
    {
    uint32_t uCoilWiper;
    if( FucCVTarget > FucCVCurrent )
      uCoilWiper = FucCVCurrent + 1;
    else if( 0 < FucCVCurrent )
      uCoilWiper = FucCVCurrent - 1;
    else
      return ;
  
    if( 0 == MCP401X_SetWiper( TOKEN_BRDSET_SET(uCoilWiper) ) )
      {
      // MCP401X_SetWiper返回0，意味着读回的值与写入的一致
      FucCVCurrent = uCoilWiper;
  
//      _SetCommonReg( REG_CM_COIL_Volt, BoardCtrl_GetInvertorVoltage() );
      _SetRealReg( REG_RL_COIL_Volt, BoardCtrl_GetInvertorVoltage() );
      }
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 启动逆变器电源
void BoardCtrl_InvertorOn(uint32_t uToken)
{
  
  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    Invertor_ON;
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 关闭逆变器电源
void BoardCtrl_InvertorOff(uint32_t uToken)
{
  
  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    Invertor_OFF;
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读逆变器电源状态
int BoardCtrl_GetInvertorPower(void)
{
  int iRes = Invertor_State;
  if( 0 != iRes )
    iRes = Invertor5V_State;
  
  return iRes? STATE_TRUE : STATE_FALSE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
         Vpeak   |
                |-|
           R0   | |
                |-|
         Vfb     |---- |---+
                |-|   |-|  |
           Ra   | |   | |<-|
                |-|   |-|
                 |-----|
                |-|
           Rb   | |
                |-|
                 |
                ---
                 -
*/
//  for( unsigned int uVset = 140; uVset <= 450; uVset += 20 )
//    {
//    float fVpeak = uVset * 1.414213562373f;
//    float fRdm = (cfVFB * cfRES0 - (fVpeak - cfVFB) * cfRESb) * cfRESa / 
//                ((fVpeak - cfVFB) * (cfRESa + cfRESb) - cfVFB * cfRES0);
//    int iWiper = fRdm * 127.0f / cfRhmax + 0.5f;
//    float fRheostat = iWiper * cfRhmax / 127.0f;
//    float fRx = 1 / (1.0 / cfRESa + 1.0 / fRheostat);
//    float fVfb = fVpeak * (fRx + cfRESb) / (fRx + cfRESb + cfRES0); // 反馈电压

//    float fVopk = cfVFB * (fRx + cfRESb + cfRES0) / (fRx + cfRESb),
//          fVout = fVopk / 1.414213562373f;
//
//    printf("Vset=%uV Vsp=%0.1fV 电位器需求=%0.2fK Wiper=%3u 数字电位器=%0.2fK"
//           " R并==%0.2fK Vfb=%0.2fV Vout=%0.1fV 误差=%0.2f%%\n", 
//           uVset, fVpeak, fRd, iWiper, fRheostat, fRx, fVfb, 
//           fVout, (fVout - uVset) * 100.0 / uVset );
//    }
//Vset=140V Vsp=198.0V R需求=8.31K Wiper=106 Rheo=8.35K R并==4.55K Vout=139.7V 误差=-0.19%
//Vset=160V Vsp=226.3V R需求=6.23K Wiper= 79 Rheo=6.22K R并==3.83K Vout=160.1V 误差=0.04%
//Vset=180V Vsp=254.6V R需求=4.91K Wiper= 62 Rheo=4.88K R并==3.28K Vout=180.5V 误差=0.30%
//Vset=200V Vsp=282.8V R需求=4.00K Wiper= 51 Rheo=4.02K R并==2.87K Vout=199.7V 误差=-0.15%
//Vset=220V Vsp=311.1V R需求=3.34K Wiper= 42 Rheo=3.31K R并==2.49K Vout=221.2V 误差=0.56%
//Vset=240V Vsp=339.4V R需求=2.84K Wiper= 36 Rheo=2.83K R并==2.21K Vout=240.1V 误差=0.05%
//Vset=260V Vsp=367.7V R需求=2.44K Wiper= 31 Rheo=2.44K R并==1.96K Vout=259.9V 误差=-0.02%
//Vset=280V Vsp=396.0V R需求=2.12K Wiper= 27 Rheo=2.13K R并==1.75K Vout=279.5V 误差=-0.18%
//Vset=300V Vsp=424.3V R需求=1.85K Wiper= 24 Rheo=1.89K R并==1.59K Vout=297.0V 误差=-0.99%
//Vset=320V Vsp=452.5V R需求=1.63K Wiper= 21 Rheo=1.65K R并==1.42K Vout=317.8V 误差=-0.68%
//Vset=340V Vsp=480.8V R需求=1.44K Wiper= 18 Rheo=1.42K R并==1.24K Vout=342.8V 误差=0.83%
//Vset=360V Vsp=509.1V R需求=1.28K Wiper= 16 Rheo=1.26K R并==1.12K Vout=362.5V 误差=0.70%
//Vset=380V Vsp=537.4V R需求=1.14K Wiper= 14 Rheo=1.10K R并==0.99K Vout=385.3V 误差=1.40%
//Vset=400V Vsp=565.7V R需求=1.01K Wiper= 13 Rheo=1.02K R并==0.93K Vout=398.1V 误差=-0.47%
//Vset=420V Vsp=594.0V R需求=0.90K Wiper= 11 Rheo=0.87K R并==0.80K Vout=427.1V 误差=1.68%
//Vset=440V Vsp=622.3V R需求=0.80K Wiper= 10 Rheo=0.79K R并==0.73K Vout=443.6V 误差=0.81%
//-----------------------------------------------------------------------------
static constexpr float cfRES0  = 360.0f,  
                       cfRESa  = 10.0f,
                       cfRESb  = 1.0f,
                       cfVFB   = 3.0f,     // 反馈点电压
                       cfRhmax = 10.0f;    // 变阻器最大值
//-----------------------------------------------------------------------------
// 设置逆变器输出电压
void BoardCtrl_SetInvertorVoltage(uint32_t uToken, bool bImme)
{

  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    uint32_t uVset = TOKEN_BRDSET_GET( uToken );

    // 电压峰值
    float fVpeak = uVset * 1.414213562373f;
    // 对变阻器的阻值需求
    float fRdm =  (cfVFB * cfRES0  - (fVpeak - cfVFB)  * cfRESb) * cfRESa / 
                 ((fVpeak - cfVFB) * (cfRESa + cfRESb) - cfVFB   * cfRES0);

    if( fRdm <= 0 )
      fRdm = 0.0f;
    else if( fRdm > cfRhmax )
      fRdm = cfRhmax;

    // 计算变阻器档位
    uint32_t uWiper = fRdm * MAX_WIPER / cfRhmax + 0.5f;
#ifdef USE_DEV_ASSERT
    // 边界检查
    DEV_ASSERT( MIN_WIPER > uWiper || MAX_WIPER < uWiper, GFC_ErrValue );
#endif

    if( MIN_WIPER > uWiper )
      uWiper = MIN_WIPER;
    else if( MAX_WIPER < uWiper )
      uWiper = MAX_WIPER;

    // 是否要立即写入
    if( true == bImme )
      {
      if( 0 == MCP401X_SetWiper( TOKEN_BRDSET_SET(uWiper) ) )
        {
        // MCP401X_SetWiper返回0，意味着读回的值与写入的一致
        FucCVCurrent = uWiper;

        _SetRealReg( REG_RL_COIL_Volt, BoardCtrl_GetInvertorVoltage() );
        }
      }

    FucCVTarget = uWiper;
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读取当前逆变器输出电压
float BoardCtrl_GetInvertorVoltage(void)
{

  uint32_t uWiper = FucCVCurrent;
  
  float rVoltage;
  if( 1 == Invertor5V_State )
    {
    float fRheostat = uWiper * cfRhmax / 127.0f,
          fRx = 1 / (1.0 / cfRESa + 1.0 / fRheostat),
          fVopk = cfVFB * (fRx + cfRESb + cfRES0) / (fRx + cfRESb),
          fVout = fVopk / 1.414213562373f;
    
    rVoltage = fVout;
    }
  else
    rVoltage = 0;

  return rVoltage;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 出口继电器控制
void BoardCtrl_SetRelay(uint32_t uToken)
{

  if( TOKEN_BRDCTRL_ISCORRECT(uToken) )
    {
    uint32_t uRelayIdx = TOKEN_BRDCTRL_IDX(uToken),
             uAction   = TOKEN_BRDCTRL_OP(uToken);
    if( TOKEN_BRDCTRL_CK1 == uRelayIdx )
      {
      if( TOKEN_BRDCTRL_ON == uAction )
        {
        Trip1_ON;

        if( STATE_TRUE != _GetIOStateReg(REG_RELAY0) )
          {
          _SetIOStateReg(REG_RELAY0, STATE_TRUE);

          EVTMGR_AppendEvent( REG_RELAY0, STATE_TRUE );
          }
        }
      else if( TOKEN_BRDCTRL_OFF == uAction )
        {
        Trip1_OFF;

        if( STATE_FALSE != _GetIOStateReg(REG_RELAY0) )
          {
          _SetIOStateReg(REG_RELAY0, STATE_FALSE);

          EVTMGR_AppendEvent( REG_RELAY0, STATE_FALSE );
          }
        }
      else
        {
        DEV_FAULT( GFC_ErrParam );
        }
      }
    else
      {
      DEV_FAULT( GFC_ErrParam );
      }
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读出口继电器状态
int BoardCtrl_GetRelayState(void)
{

  return Trip1_State? STATE_TRUE : STATE_FALSE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 线圈旁路供电方式
//   uToken： 宏TOKEN_BRDCTRL_ENCODE
void BoardCtrl_SetPassbyRelay( uint32_t uToken)
{
  
  if( TOKEN_BRDCTRL_ISCORRECT(uToken) )
    {
    uint32_t uRelayIdx = TOKEN_BRDCTRL_IDX(uToken),
             uAction   = TOKEN_BRDCTRL_OP(uToken);
    
    if( TOKEN_BRDCTRL_PASSBY == uRelayIdx )
      {
      if( TOKEN_BRDCTRL_ON == uAction )
        {
        ACout_Passby_ON;

        if( STATE_TRUE != _GetIOStateReg(REG_RELAY1) )
          {
          _SetIOStateReg(REG_RELAY1, STATE_TRUE);

          EVTMGR_AppendEvent( REG_RELAY1, STATE_TRUE );
          }
        }
      else if( TOKEN_BRDCTRL_OFF == uAction )
        {
        ACout_Passby_OFF;

        if( STATE_FALSE != _GetIOStateReg(REG_RELAY1) )
          {
          _SetIOStateReg(REG_RELAY1, STATE_FALSE);

          EVTMGR_AppendEvent( REG_RELAY1, STATE_FALSE );
          }
        }
      else
        {
        DEV_FAULT( GFC_ErrParam );
        }
      }
    else
      {
      DEV_FAULT( GFC_ErrParam );
      }
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读线圈旁路供电方式
int BoardCtrl_GetPassby(void)
{

  return ACout_Passby_State? STATE_TRUE : STATE_FALSE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 加热器控制
void BoardCtrl_HeaterControl(uint32_t uToken)
{

  if( TOKEN_BRDCTRL_ISCORRECT(uToken) )
    {
    uint32_t uCtrlIdx = TOKEN_BRDCTRL_IDX(uToken),
             uAction   = TOKEN_BRDCTRL_OP(uToken);
    if( TOKEN_BRDCTRL_HEATER == uCtrlIdx )
      {
      if( TOKEN_BRDCTRL_ON == uAction )
        {
        Heater_ON;

        if( STATE_TRUE != _GetIOStateReg(REG_RELAY2) )
          {
          _SetIOStateReg(REG_RELAY2, STATE_TRUE);

          EVTMGR_AppendEvent( REG_RELAY2, STATE_TRUE );
          }
        }
      else if( TOKEN_BRDCTRL_OFF == uAction )
        {
        Heater_OFF;

        if( STATE_FALSE != _GetIOStateReg(REG_RELAY2) )
          {
          _SetIOStateReg(REG_RELAY2, STATE_FALSE);

          EVTMGR_AppendEvent( REG_RELAY2, STATE_FALSE );
          }
        }
      else
        {
        DEV_FAULT( GFC_ErrParam );
        }
      }
    else
      {
      DEV_FAULT( GFC_ErrParam );
      }
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读加热器状态
int BoardCtrl_GetHeater(void)
{

  return Heater_State? STATE_TRUE : STATE_FALSE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 风扇控制
void BoardCtrl_FanControl(uint32_t uToken)
{

  if( TOKEN_BRDCTRL_ISCORRECT(uToken) )
    {
    uint32_t uCtrlIdx = TOKEN_BRDCTRL_IDX(uToken),
             uAction  = TOKEN_BRDCTRL_OP(uToken);
    if( TOKEN_BRDCTRL_FAN == uCtrlIdx )
      {
      if( TOKEN_BRDCTRL_ON == uAction )
        {
        DevFanCtrl_ON;

        if( STATE_TRUE != _GetIOStateReg(REG_RELAY3) )
          {
          _SetIOStateReg(REG_RELAY3, STATE_TRUE);

          EVTMGR_AppendEvent( REG_RELAY2, STATE_TRUE );
          }
        }
      else if( TOKEN_BRDCTRL_OFF == uAction )
        {
        DevFanCtrl_OFF;

        if( STATE_FALSE != _GetIOStateReg(REG_RELAY3) )
          {
          _SetIOStateReg(REG_RELAY3, STATE_FALSE);

          EVTMGR_AppendEvent( REG_RELAY3, STATE_FALSE );
          }
        }
      else
        {
        DEV_FAULT( GFC_ErrParam );
        }
      }
    else
      {
      DEV_FAULT( GFC_ErrParam );
      }
    }
  else
    {
    DEV_FAULT( GFC_ErrToken );
    }
}
//-----------------------------------------------------------------------------
// 读风扇状态
int  BoardCtrl_GetFanControl(void)
{
  
  return DevFanCtrl_State? STATE_TRUE : STATE_FALSE;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 关闭装置电源
void BoardCtrl_Shutdown(uint32_t uToken)
{

  if( TOKEN_BRDSET_ISCORRECT(uToken) )
    {
    KeepMCUPower_OFF;
    }
}
//-----------------------------------------------------------------------------
