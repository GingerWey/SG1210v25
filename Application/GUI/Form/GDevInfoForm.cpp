//-----------------------------------------------------------------------------
/*
 File        : GPDevInfoForm.c
 Version     : V1.10
 By          : 银网科技

 Description :装置参数窗体
        
 Date       : 2023.12.05
 
 2024/10/5
 Ver 2.1  Wey
   适配V2.1硬件
*/
//-----------------------------------------------------------------------------
#include "GDevInfoForm.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "GUIMessage.h"
#include "FontSGRes.h"

#include "GUIBitMap.h"
#include "PictureRes.h"

#include "CRC1632.h"
#include "DevRegs.h"
#include "GPVersion.h"


#include "RamHeap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __vmSIMULATOR__
#include "rtc.h"
#include "SD3077.h"
#include "SPIFlash.h"
#include "gpio.h"

#include <cmsis_os.h>
#include <stm32f4xx_hal.h>
#endif
#include <DevIntf.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define  GetRegisterValue(x)      DevConfig.Configs[(x) - REG_DEVCONFIG]
//-----------------------------------------------------------------------------
#define SIZE_ChipFlash            (*(uint16_t*)FLASHSIZE_BASE * 1024)
//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define NUM_PAGES          4
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     32

#define WIDTH_EDGE         4

#define HEIGHT_INFOITEM   22
//-----------------------------------------------------------------------------
#define Caption_x1         0
#define Caption_y1         0
#define Caption_x2        (DESKTOP_WIDTH - 1)
#define Caption_y2        (Caption_x1 + HIGHT_CAPTION - 1)
                          
#define CapLabel_x1       (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1       (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2       (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2       (Caption_y2 - WIDTH_EDGE)

// Info List
#define InfoPanel_x1      (0 + WIDTH_EDGE)
#define InfoPanel_y1      (Caption_y2 + WIDTH_EDGE)
#define InfoPanel_x2      (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define InfoPanel_y2      (DESKTOP_HEIGHT - WIDTH_EDGE - 1)
                          
#define InfoView_x1       (InfoPanel_x1 + WIDTH_EDGE)
#define InfoView_y1       (InfoPanel_y1 + WIDTH_EDGE)
#define InfoView_x2       (InfoPanel_x2 - WIDTH_EDGE)
#define InfoView_y2       (InfoPanel_y2 - WIDTH_EDGE)

// Clock
#define ClockPanel_x1     (InfoView_x1 +  WIDTH_EDGE)
#define ClockPanel_y1     (InfoView_y1 +  WIDTH_EDGE)
#define ClockPanel_x2     (InfoView_x2 -  WIDTH_EDGE)
#define ClockPanel_y2     (ClockPanel_y1 +  20 + WIDTH_EDGE * 2)
                          
#define ClockLabel_x1     (ClockPanel_x1 + WIDTH_EDGE)
#define ClockLabel_y1     (ClockPanel_y1 + WIDTH_EDGE)
#define ClockLabel_x2     (ClockPanel_x2 - WIDTH_EDGE)
#define ClockLabel_y2     (ClockPanel_y2 - WIDTH_EDGE)
                          
#define ftClockLabel      &GUI_Font24B_ASCII
#define crClockPanel      crInfoBkg
#define crClockLabel      GUI_LIGHTCYAN

// Colors
#define crFormBkg         0x001F1F1F  
#define crFrameHigh       0x002F2F2F  
#define crFrameDrak       0x000F0F0F
                          
#define crCaptionFont     GUI_GRAY_9A  
#define ftCaption         GUI_FONT_24LTH_CHN

#define crInfoBkg         0x00181818 

#define crInfoText        GUI_ORANGE
#define crInfoTextInact   0x002F2F2F
#define ftInfoText        GUI_FONT_16LTH_CHN
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 列表状态数据
typedef struct tagFormState
{

  uint8_t     FTopItem;
  uint8_t     FCurItem;
  uint16_t    FCount;
  
  uint16_t    FTick;
  uint32_t    FNextTick;
  
  uint8_t     FPageIndex;

  const GWinForm *pDialog;
} GFormState;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 列表状态
static GFormState m_FormState = {0};
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
#ifndef __vmSIMULATOR__
    osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  GUI_SetBkColor( crFormBkg );
  GUI_Clear();
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT Rect1 = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
  GUI_DrawRectEx( &Rect1 );

  GUI_SetColor( crCaptionFont );
  GUI_SetFont( ftCaption ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
  GUI_DispStringInRect( "装置信息", &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
#endif

  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( InfoPanel_x1, InfoPanel_y1, InfoPanel_x2, InfoPanel_y2 );

#ifndef __vmSIMULATOR__
  osMutexRelease(FGUIState.mutexGUI);
#endif
}
//-----------------------------------------------------------------------------
static void _DrawPageSwitchSign(uint32_t uForward)
{
  
  // Next page sign
  constexpr GUI_POINT ptRtArrow[3] = 
     { { InfoView_x2 - 6,  InfoView_y2 - 6},
       { InfoView_x2 - 2,  InfoView_y2 - 4},
       { InfoView_x2 - 6,  InfoView_y2 - 2} };
  constexpr GUI_POINT ptLfArrow[3] = 
     { { InfoView_x1 + 6,  InfoView_y2 - 6},
       { InfoView_x1 + 2,  InfoView_y2 - 4},
       { InfoView_x1 + 6,  InfoView_y2 - 2} };
  
  GUI_SetPenSize( 1 );
  GUI_SetPenShape( GUI_PS_SQUARE );
  GUI_SetLineStyle( GUI_LS_SOLID ); 

  if( 0 != (uForward & 0x01) )
    {    
    GUI_DrawLine(ptRtArrow[0].x, ptRtArrow[0].y, ptRtArrow[1].x, ptRtArrow[1].y);
    GUI_DrawLine(ptRtArrow[1].x, ptRtArrow[1].y, ptRtArrow[2].x, ptRtArrow[2].y);
    GUI_DrawLine(ptRtArrow[2].x, ptRtArrow[2].y, ptRtArrow[0].x, ptRtArrow[0].y);
    }
  else if( 0 != (uForward & 0x10) )
    {    
    GUI_DrawLine(ptLfArrow[0].x, ptLfArrow[0].y, ptLfArrow[1].x, ptLfArrow[1].y);
    GUI_DrawLine(ptLfArrow[1].x, ptLfArrow[1].y, ptLfArrow[2].x, ptLfArrow[2].y);
    GUI_DrawLine(ptLfArrow[2].x, ptLfArrow[2].y, ptLfArrow[0].x, ptLfArrow[0].y);
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _DrawPage1()
{

  GUI_SetColor( crInfoBkg );
  GUI_FillRect( InfoView_x1, InfoView_y1, InfoView_x2, InfoView_y2 );  

  if( 0 == m_FormState.pDialog )
    GUI_SetColor( crInfoText );
  else
    GUI_SetColor( crInfoTextInact );

  GUI_SetFont ( ftInfoText ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );

  int iInfo_x1 = InfoView_x1 + WIDTH_EDGE * 4,
      iInfo_y1 = InfoView_y1 + WIDTH_EDGE * 2;

  if( 0 == m_FormState.pDialog )
    GUI_SetColor( crInfoText );
  else
    GUI_SetColor( crInfoTextInact );
  
  GUI_SetFont ( ftInfoText ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
  char szStr[64];
  sprintf( szStr, "软件版本: %d.%d.%d.%d %s", 
           Firmware_Ver / 100, (Firmware_Ver % 100), 
           Firmware_Evo, Firmware_Impr,
           __DATE__  );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
  
  uint32_t uCRC32 = _GetCommonReg( REG_FWCRC32 );
  if( 0 == uCRC32 )
    {
//    uCRC32 = CRC32( (uint8_t*)SCB->VTOR, 
//                    (uint32_t)(SIZE_ChipFlash - (SCB->VTOR - FLASH_BASE)),
//                    (uint32_t)-1 
//                   );
    uCRC32 = CRC32_FWHW();
    _SetCommonReg( REG_FWCRC32, uCRC32 );
    }

  sprintf( szStr, "软件校验: %04X-%04X", 
           uCRC32 >> 16, uCRC32 & 0xFFFF );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

  sprintf( szStr, "硬件版本: %u.%u.%u", 
                    Hardware_Ver / 1000000, 
                   (Hardware_Ver % 1000000) / 10000,
                   (Hardware_Ver % 10000) );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

#ifndef __vmSIMULATOR__
  uCRC32 = CRC32((void*)UID_BASE, 12, -1ul) ^ 0x5A5A3C3C;
  sprintf( szStr, "硬件标识: %04X-%04X", 
           uCRC32 >> 16, uCRC32 & 0xFFFF );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
#endif

#ifndef __vmSIMULATOR__
  SPIFLASH_Init();
  TFlashParameter paramFlash;
  SPIFLASH_GetFlashParam( &paramFlash );
  sprintf( szStr, "硬盘标识: %04X-%04X", 
           paramFlash.Type >> 16, paramFlash.Type & 0xFFFF );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  sprintf( szStr, "容量: %dM", 
           paramFlash.Capacity / (1024 * 1024) );
  GUI_DispStringAt( szStr, iInfo_x1 + 160, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
  
  osVersion_t osVer;
  osKernelGetInfo( &osVer, szStr, sizeof(szStr) );
  GUI_DispStringAt( szStr + 4, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
#endif
  
  sprintf( szStr, "GUI: %s",
                   GUI_GetVersionString() );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );

#ifndef __vmSIMULATOR__
  uint32_t uHALVer = HAL_GetHalVersion();
  sprintf( szStr, "HAL: %u.%u.%u.%u",
                    uHALVer >> 24,
                    (uHALVer >> 16) & 0xFF,
                    (uHALVer >>  8) & 0xFF,
                    uHALVer & 0xFF );
  GUI_DispStringAt( szStr, iInfo_x1 + 160, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
  
  sprintf( szStr, "CMSIS: %u.%u",
                    __CM_CMSIS_VERSION_MAIN,
                    __CM_CMSIS_VERSION_SUB );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
#endif
  
  #ifdef __clang__
    {
    uint32_t uCLangVer = __ARMCC_VERSION;
    sprintf( szStr, "CLANG: %u.%u.%u", 
                    uCLangVer / 1000000,
                    (uCLangVer % 1000000) / 10000,
                    (uCLangVer % 10000) );
    GUI_DispStringAt( szStr, iInfo_x1 + 160, iInfo_y1 );
    }
  #elif defined(__GNUC__)
    GUI_DispStringAt( " GUNC: v5.06", iInfo_x1, iInfo_y1 );
  #endif

  // --------- 
  GUI_SetColor( crFrameHigh );
  _DrawPageSwitchSign( 0x01 );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _DrawClock()
{
  
  if( 1 != m_FormState.FPageIndex )
    return ;
  
  if( 0 == m_FormState.pDialog )
    GUI_SetColor( crClockLabel );
  else
    GUI_SetColor( crInfoTextInact );
  
  GUI_SetBkColor( crClockPanel );
  GUI_SetFont( ftClockLabel); 
  GUI_SetTextMode( GUI_TEXTMODE_NORMAL );

  char szStr[32];
  TDateTimeType dtNow;
#ifndef __vmSIMULATOR__
  // RTC_GetTime( &dtNow );
  SD3077_GetDateTime( &dtNow );
#else
  DevIntf_getDateTime(&dtNow);
#endif
  sprintf( szStr, "%04u-%02u-%02u %02u:%02u:%02u", 
           dtNow.Year,  dtNow.Month,   dtNow.Day, 
           dtNow.Hours, dtNow.Minutes, dtNow.Seconds );
  
  GUI_RECT Rect1 = { ClockLabel_x1, ClockLabel_y1, ClockLabel_x2, ClockLabel_y2 };
  GUI_DispStringInRect( szStr, &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _DrawPage2()
{
    
  // 
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( InfoView_x1, InfoView_y1, InfoView_x2, InfoView_y2 );
  
  // Clock
  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( ClockPanel_x1, ClockPanel_y1, ClockPanel_x2, ClockPanel_y2 );
  GUI_SetColor( crClockPanel );
  GUI_FillRect( ClockLabel_x1, ClockLabel_y1, ClockLabel_x2, ClockLabel_y2 );
  
  _DrawClock();
  
  // dev infos
  int iInfo_x1 = InfoView_x1   + WIDTH_EDGE * 4,
      iInfo_y1 = ClockPanel_y2 + WIDTH_EDGE * 2;

  GUI_SetFont ( ftInfoText );
  GUI_SetColor( crInfoText );
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
#ifndef __vmSIMULATOR__
  char szStr[64];
  uint8_t ucBuf[16];
  int iRes = SD3077_ReadSN( ucBuf );
  if( 0 == iRes )
    {
    sprintf( szStr, "标识：%02u/%02u/%02u@%02u %02X%02X-%02X%02X", 
                    ucBuf[0], ucBuf[1], ucBuf[2], ucBuf[3],
                    ucBuf[4], ucBuf[5], ucBuf[6], ucBuf[7] );
    }
  else
    {
    sprintf( szStr, "标识：failed" ); 
    }
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
  
  int iValue;
  iRes = SD3077_GetTemperature( &iValue );
  if( 0 == iRes )
    {
    sprintf( szStr, "温度：%d℃", iValue );
    }
  else
    {
    sprintf( szStr, "温度：failed" ); 
    }
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

  iRes = SD3077_ReadBVol( &iValue );
  if( 0 == iRes )
    {
    sprintf( szStr, "电池：%0.2fV", float(iValue) / 100.0f );
    }
  else
    {
    sprintf( szStr, "电池：failed" ); 
    }
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
    
  TDateTimeType dtTime;
  iRes = SD3077_GetLowestTemp( &iValue, &dtTime );
  if( 0 == iRes )
    {
    sprintf( szStr, "最冷：%d℃ %04u-%02u-%02u %02u:%02u", iValue, 
                    dtTime.Year,  dtTime.Month,  dtTime.Day, 
                    dtTime.Hours, dtTime.Minutes );
    }
  else
    {
    sprintf( szStr, "最冷：failed" ); 
    }
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

  iRes = SD3077_GetHighestTemp( &iValue, &dtTime );
  if( 0 == iRes )
    {
    sprintf( szStr, "最热：%d℃ %04u-%02u-%02u %02u:%02u", iValue, 
                    dtTime.Year,  dtTime.Month,  dtTime.Day, 
                    dtTime.Hours, dtTime.Minutes );
    }
  else
    {
    sprintf( szStr, "最热：failed" ); 
    }
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;
#endif
  // --------- 
  GUI_SetColor( crFrameHigh );
  _DrawPageSwitchSign( 0x01 );
  _DrawPageSwitchSign( 0x10 );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _updatePage3()
{

  GUI_SetFont ( ftInfoText );
  GUI_SetTextMode( GUI_TEXTMODE_NORMAL );

  GUI_SetPenShape( GUI_PS_SQUARE );
  GUI_SetLineStyle( GUI_LS_SOLID );
  GUI_SetPenSize( 1 );

  int iInfo_x1 = InfoView_x1 + WIDTH_EDGE * 4,
      iInfo_y1 = InfoView_y1 + WIDTH_EDGE * 2;

  // 锂电池充电侧
//  float fVbus, fVshunt, fIbus, fPower;
//  int iValue;
//  iValue  = (int)_GetCommonReg( REG_CM_BCHRG_Ushunt );
//  fVshunt = (float)(iValue) / RATIO_Voltage;
//  iValue  = (int)_GetCommonReg( REG_CM_BCHRG_Ubus );
//  fVbus   = (float)(iValue) / RATIO_Voltage;
//  iValue  = (int)_GetCommonReg( REG_CM_BCHRG_Ibus );
//  fIbus   = (float)(iValue) / RATIO_Current;
//  iValue  = (int)_GetCommonReg( REG_CM_BCHRG_Pbus );
//  fPower  = (float)(iValue) / RATIO_Power;
  float fVbus = _GetRealReg(REG_RL_BCHRG_Ubus),
      fVshunt = _GetRealReg(REG_RL_BCHRG_Ushunt),
        fIbus = _GetRealReg(REG_RL_BCHRG_Ibus),
       fPower = _GetRealReg(REG_RL_BCHRG_Pbus);
  
  #define info_c1xc1        (iInfo_x1 + 90)
  #define info_c2x1         (iInfo_x1 + 128)
  #define info_c2x2         (iInfo_x1 + 130)
  #define info_c2x3         (InfoView_x2 - 18)
  #define info_c2x4         (InfoView_x2 - 16)
  #define info_c2barx2(p)   (info_c2x2 + (info_c2x3 - info_c2x2) * (p))
  
#if VER_SG12B20 >= 200
  #define BATVOLT_ULIMIT1    15.4f
  #define BATVOLT_DLIMIT1    10.0f
  #define BATVOLT_ULIMIT2    14.6f
  #define BATVOLT_DLIMIT2    10.0f
#else
  #define BATVOLT_ULIMIT    8.3f
  #define BATVOLT_DLIMIT    6.4f
#endif
  
  //----------------------------------------------
  char szStr[64];
  sprintf( szStr, "电压：%0.2fV", fVbus );
  GUI_SetColor( crInfoText );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  
  GUI_DrawRect( info_c2x1, iInfo_y1 + 2, info_c2x4, iInfo_y1 + 16 );
  GUI_SetColor( crInfoBkg );

#ifndef __vmSIMULATOR__
  int iBarX2;
  // 是否在充电状态
  if( 0 != Adapter_ON_State && 0 > fIbus )
    iBarX2 = info_c2barx2( (fVbus - BATVOLT_DLIMIT1) / 
                           (BATVOLT_ULIMIT1 - BATVOLT_DLIMIT1) );
  else
    iBarX2 = info_c2barx2( (fVbus - BATVOLT_DLIMIT2) / 
                           (BATVOLT_ULIMIT2 - BATVOLT_DLIMIT2) );
  if( iBarX2 > info_c2x4 )
    iBarX2 = info_c2x4;
    
  if( fVbus > BATVOLT_DLIMIT1 )
    {
    GUI_FillRect( iBarX2,    iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  sprintf( szStr, "电流：%0.2fmA", fIbus );
  GUI_SetColor( crInfoText );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );

  GUI_DrawRect( info_c2x1, iInfo_y1 + 2, info_c2x4, iInfo_y1 + 16 );
  if( fIbus > 0 )
    {
    iBarX2 = info_c2barx2( fIbus / 4100.0f );
    if( iBarX2 > info_c2x4 )
      iBarX2 = info_c2x4;
    GUI_SetColor( crInfoBkg );
    GUI_FillRect( iBarX2,    iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  else if( fIbus < 0 )
    {
    fIbus = -fIbus;
    iBarX2 = info_c2barx2( fIbus / 2500.0f );
    if( iBarX2 > info_c2x4 )
      iBarX2 = info_c2x4;
    GUI_SetColor( crInfoBkg );
    GUI_FillRect( iBarX2,   iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  GUI_SetColor( crInfoText );
  sprintf( szStr, "分压：%0.2fmV", fVshunt );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  GUI_SetColor( crInfoText );
  if( fVshunt >= 0 )
    sprintf( szStr, "功率：%0.3fW", fPower / 1000.0f);
  else
    sprintf( szStr, "充电：%0.3fW", fPower / 1000.0f);
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  GUI_SetColor( crInfoText );
//  iValue = (int)_GetCommonReg( REG_CM_BCHRG_Level );
//  float fBatCap  = (float)(iValue) / RATIO_Percentage * 100.0f;
  auto fBatCap = _GetRealReg( REG_RL_BAT_CAPLevel );
  sprintf( szStr, "电量：%0.2f%%", fBatCap );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );

  // 电池容量指示
  GUI_DrawRect( info_c2x1, iInfo_y1 + 2, info_c2x4, iInfo_y1 + 16 );
  GUI_SetColor( crInfoBkg );
  iBarX2 = info_c2barx2( fBatCap / 100 );
  if( iBarX2 > info_c2x4 )
    iBarX2 = info_c2x4;
  GUI_FillRect( iBarX2,    iInfo_y1 + 4, 
                info_c2x3, iInfo_y1 + 14 );
  GUI_SetColor( crInfoText );
  if( fBatCap > 0 )
    {
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  
  // v2.1
  iInfo_y1 += HEIGHT_INFOITEM * 3 / 2;
  GUI_SetColor( crFrameHigh );
  GUI_DrawLine( InfoView_x1, iInfo_y1 - HEIGHT_INFOITEM / 3, 
                InfoView_x2, iInfo_y1 - HEIGHT_INFOITEM / 3 );
  //==================================================
  // 电池放电侧
//  iValue  = (int)_GetCommonReg( REG_CM_BTOUT_Ubus );
//  fVbus   = (float)(iValue) / RATIO_Voltage;
//  iValue  = (int)_GetCommonReg( REG_CM_BTOUT_Ibus );
//  fIbus   = (float)(iValue) / RATIO_Current;
//  iValue  = (int)_GetCommonReg( REG_CM_BTOUT_Pbus );
//  fPower  = (float)(iValue) / RATIO_Power;
   fVbus   = _GetRealReg(REG_RL_BTOUT_Ubus),
   fVshunt = _GetRealReg(REG_RL_BTOUT_Ushunt),
   fIbus   = _GetRealReg(REG_RL_BTOUT_Ibus),
   fPower  = _GetRealReg(REG_RL_BTOUT_Pbus);
    
  //----------------------------------------------
  sprintf( szStr, "电压：%0.2fV", fVbus );
  GUI_SetColor( crInfoText );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
  
  GUI_DrawRect( info_c2x1, iInfo_y1 + 2, info_c2x4, iInfo_y1 + 16 );
  GUI_SetColor( crInfoBkg );
  // 是否在充电状态
  if( 0 != Adapter_ON_State && 0 > fIbus )
    iBarX2 = info_c2barx2( (fVbus - BATVOLT_DLIMIT1) / 
                           (BATVOLT_ULIMIT1 - BATVOLT_DLIMIT1) );
  else
    iBarX2 = info_c2barx2( (fVbus - BATVOLT_DLIMIT2) / 
                           (BATVOLT_ULIMIT2 - BATVOLT_DLIMIT2) );
  if( iBarX2 > info_c2x4 )
    iBarX2 = info_c2x4;
 if( fVbus > BATVOLT_DLIMIT1 )
    {
    GUI_FillRect( iBarX2,    iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );

    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  sprintf( szStr, "电流：%0.3fA", fIbus / 1000.0f);
  GUI_SetColor( crInfoText );
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );

  GUI_DrawRect( info_c2x1, iInfo_y1 + 2, info_c2x4, iInfo_y1 + 16 );
  if( fIbus > 0 )
    {
    iBarX2 = info_c2barx2( fIbus / 32000.0f );
    if( iBarX2 > info_c2x4 )
      iBarX2 = info_c2x4;

    GUI_SetColor( crInfoBkg );
    GUI_FillRect( iBarX2,    iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  else if( fIbus < 0 )
    {
    fIbus = -fIbus;
    iBarX2 = info_c2barx2( fIbus / 8000.0f );
    if( iBarX2 > info_c2x4 )
      iBarX2 = info_c2x4;
    GUI_SetColor( crInfoBkg );
    GUI_FillRect( iBarX2,   iInfo_y1 + 4, 
                  info_c2x3, iInfo_y1 + 14 );
    GUI_SetColor( crInfoText );
    GUI_FillRect( info_c2x2, iInfo_y1 + 4, iBarX2, iInfo_y1 + 14 );
    }
  iInfo_y1 += HEIGHT_INFOITEM;

  //----------------------------------------------
  GUI_SetColor( crInfoBkg );
  GUI_FillRect( info_c1xc1, iInfo_y1, 
                info_c2x1,  iInfo_y1 + HEIGHT_INFOITEM - 1 );
  GUI_SetColor( crInfoText );
  sprintf( szStr, "功率：%0.3fW", fPower / 1000.0f);
  GUI_DispStringAt( szStr, iInfo_x1, iInfo_y1 );
#endif
}
//-----------------------------------------------------------------------------
static void _DrawPage3()
{

  GUI_SetColor( crInfoBkg );
  GUI_FillRect( InfoView_x1, InfoView_y1, InfoView_x2, InfoView_y2 );  

  _updatePage3();

  // --------- 
  GUI_SetColor( crFrameHigh );
  _DrawPageSwitchSign( 0x01 );
  _DrawPageSwitchSign( 0x10 );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _DrawPage4()
{

  GUI_SetColor( crInfoBkg );
  GUI_FillRect( InfoView_x1, InfoView_y1, InfoView_x2, InfoView_y2 );  

  if( 0 == m_FormState.pDialog )
    GUI_SetColor( crInfoText );
  else
    GUI_SetColor( crInfoTextInact );

  GUI_SetFont ( ftInfoText ); 
  GUI_SetColor( crInfoText );
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );

  constexpr int iInfo_ColWidth = (InfoView_x2 - InfoView_x1 - WIDTH_EDGE * 3) / 3,
                iInfo_x1   = InfoView_x1 + WIDTH_EDGE,
                iInfo_y1   = InfoView_y1,
                iInfo_c1x1 = iInfo_x1,
                iInfo_c1x2 = iInfo_c1x1 + iInfo_ColWidth - WIDTH_EDGE,
                iInfo_c2x1 = iInfo_c1x1 + iInfo_ColWidth,
                iInfo_c2x2 = iInfo_c2x1 + iInfo_ColWidth - WIDTH_EDGE,
                iInfo_c3x1 = iInfo_c2x1 + iInfo_ColWidth,
                iInfo_c3x2 = iInfo_c3x1 + iInfo_ColWidth - WIDTH_EDGE;

  GUI_RECT Rect = {iInfo_c2x1, iInfo_y1, iInfo_c2x2, iInfo_y1 + HEIGHT_INFOITEM};
  GUI_DispStringInRect( "Kernel",      &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.x0 = iInfo_c3x1; Rect.x1 = iInfo_c3x2;
  GUI_DispStringInRect( "Application", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );

  Rect.x0  = iInfo_c1x1;      Rect.x1 = iInfo_c1x2;
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "剩余空间", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "最小剩余", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "分配次数", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "释放次数", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "块 数 量", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "最 大 块", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  GUI_DispStringInRect( "最 小 块", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );

  char szStr[12];
  {
#ifndef __vmSIMULATOR__
      HeapStats_t heapState;
  vPortGetHeapStats( &heapState );

  Rect.x0 = iInfo_c2x1;  Rect.x1 = iInfo_c2x2 - 24;
  Rect.y0 = iInfo_y1 + HEIGHT_INFOITEM; Rect.y1 = Rect.y0 + HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xAvailableHeapSpaceInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xMinimumEverFreeBytesRemaining );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfSuccessfulAllocations );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfSuccessfulFrees );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfFreeBlocks );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xSizeOfLargestFreeBlockInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xSizeOfSmallestFreeBlockInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
#endif
  }
  
  {
  ramHeapStats_t heapState;
  RAM_GetHeapStats ( &heapState );
  Rect.x0 = iInfo_c3x1;  Rect.x1 = iInfo_c3x2 - 16;
  Rect.y0 = iInfo_y1 + HEIGHT_INFOITEM; Rect.y1 = Rect.y0 + HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xAvailableHeapSpaceInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xMinimumEverFreeBytesRemaining );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfSuccessfulAllocations );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfSuccessfulFrees );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xNumberOfFreeBlocks );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xSizeOfLargestFreeBlockInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  Rect.y0 += HEIGHT_INFOITEM; Rect.y1 += HEIGHT_INFOITEM;
  sprintf( szStr, "%d", heapState.xSizeOfSmallestFreeBlockInBytes );
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
  }
  
  // --------- 
  GUI_SetColor( crFrameHigh );
  _DrawPageSwitchSign( 0x10 );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _UpdateInfoView()
{
  
  switch( m_FormState.FPageIndex )
    {
    case 0:
      _DrawPage1();
      break;

    case 1:
      _DrawPage2();
      break;

    case 2:
      _DrawPage3();
      break;
 
    case 3:
      _DrawPage4();
      break;
   }
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_FormState, 0, sizeof(m_FormState) );
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{
  
  _FlushForm();
  
  _UpdateInfoView();
  
  // Next Tick
  m_FormState.FNextTick = GUI_GetTime() + 500;
  m_FormState.FPageIndex = 0;
  m_FormState.FTick = 0;
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{

  uTick = GUI_GetTime();
  if( uTick > m_FormState.FNextTick )
    {
    m_FormState.FNextTick =  uTick+ 1000;
    
    m_FormState.FTick++;
      
    switch( m_FormState.FPageIndex )
      {
      case 0:
        break;

      case 1:
        {
        if( 0 == (m_FormState.FTick & 0x7) )
          _DrawPage2();
        else
          _DrawClock();
        
        break;
        }

      case 2:
        {
        _updatePage3();

        break;
        }
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{

  switch( uwKey )
    {
    case KEY_RIGHT:
      {
      if( m_FormState.FPageIndex + 1 < NUM_PAGES )
        {
        m_FormState.FPageIndex++;
        _UpdateInfoView();
        }
      
      break;
      }

    case KEY_LEFT:
      {
      if( m_FormState.FPageIndex > 0 )
        {
        m_FormState.FPageIndex--;
        _UpdateInfoView();
        }
      else
        GUIFormClose( nullptr );

      break;
      }

//    case KEY_ADD:
//    case KEY_SUB:
//      {
//      if( m_FormState.FPageIndex + 1 < NUM_PAGES )
//        m_FormState.FPageIndex++;
//      else
//        m_FormState.FPageIndex = 0;

//      _UpdateInfoView();
//      
//      break;
//      }

    case KEY_DOWN:
      {
      GUIFormClose( nullptr );
      
      break;
      }

    case KEY_ENTER:
      {
      if( 1 == m_FormState.FPageIndex )
        {
        //m_FormState.pDialog = &FTimeDialog;
        }
      else if( 2 == m_FormState.FPageIndex )
        {
#ifndef __vmSIMULATOR__
          DevChk_FlashTest();
#endif
        }

      if( nullptr != m_FormState.pDialog )
        {
        _UpdateInfoView();

        m_FormState.pDialog->pInit( &FDevInfoForm );
        m_FormState.pDialog->pShow( nullptr );
        }

      break;
      }

    case KEY_ESCAPE:
      {
      GUIFormClose( nullptr );

      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg)
{

  if( nullptr == pMsg )
    return ;

  if( nullptr != m_FormState.pDialog )
    {
    m_FormState.pDialog->pMsg( pMsg );
    }

  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      { 
      _OnTick( pMsg->Data.v );
        
      break;
      }

    case GM_KEYUP:
      {
      if( 0 != pMsg->Param )
        {
        _OnKeyUp( pMsg->Param );
          
        pMsg->MsgId = 0;
        }

      break;
      }

    case GM_DIALOG_ACCEPT:
    case GM_DIALOG_CANCEL:
      {
      if( nullptr != m_FormState.pDialog )
        m_FormState.pDialog->pClose( nullptr );
        
      m_FormState.pDialog = nullptr;

      _UpdateInfoView();

      break;
      }
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FDevInfoForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
