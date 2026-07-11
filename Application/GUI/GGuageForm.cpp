//-----------------------------------------------------------------------------
/*
 File        : GPGuageForm.c
 Version     : V1.10
 By          : 银网科技
 Description :消息窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GGuageForm.h"

#include "GUI.h"
#include "GUIConf.h"
#include "GUICntr.h"
#include "FontCHS16LTH.h"
#include "FontCHS24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"

#include "DevRegs.h"
#include "Strings/TextStrs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define FORM_HIGHT        (DESKTOP_HEIGHT / 2)
#define FORM_WIDTH        (DESKTOP_WIDTH * 4 / 5)

#define HEIGHT_CAPTION    (FORM_HIGHT / 4)
#define WIDTH_EDGE        4
//-----------------------------------------------------------------------------
// Form
#define Form_x1           ((DESKTOP_WIDTH  - FORM_WIDTH) / 2)
#define Form_y1           ((DESKTOP_HEIGHT - FORM_HIGHT) / 2)
#define Form_x2           (Form_x1 + FORM_WIDTH - 1)
#define Form_y2           (Form_y1 + FORM_HIGHT - 1)

#define crFrameBkg        GUI_MAKE_COLOR(0x002F2F2F)  
#define crFrameHigh       GUI_MAKE_COLOR(0x008F4F4F)  
#define crFrameDrak       GUI_MAKE_COLOR(0x001F1F1F)

#define crSprit           GUI_GRAY_7C

// Caption
#define Caption_x1        (Form_x1 + WIDTH_EDGE)
#define Caption_y1        (Form_y1 + (FORM_HIGHT / 2 - HEIGHT_CAPTION) / 2)
#define Caption_x2        (Form_x2 - WIDTH_EDGE)
#define Caption_y2        (Caption_y1 + HEIGHT_CAPTION - 1)

#define CapLabel_x1       (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1       (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2       (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2       (Caption_y2 - WIDTH_EDGE)

#define crCaptionFont     GUI_LIGHTCYAN  
#define ftCaption         GUI_FONT_24LTH_CHN

// Guage bar
#define GuagePanel_x1     (Form_x1 + WIDTH_EDGE * 2)
#define GuagePanel_y1     (Form_y1 + FORM_HIGHT / 2 + WIDTH_EDGE)
#define GuagePanel_x2     (Form_x2 - WIDTH_EDGE * 2)
#define GuagePanel_y2     (GuagePanel_y1 + HEIGHT_CAPTION + 1 - WIDTH_EDGE * 2)

#define Guagebar_x1       (GuagePanel_x1 + WIDTH_EDGE)
#define Guagebar_y1       (GuagePanel_y1 + WIDTH_EDGE)
#define Guagebar_x2       (GuagePanel_x2 - WIDTH_EDGE)
#define Guagebar_y2       (GuagePanel_y2 - WIDTH_EDGE)
#define Guagebar_yc       ((Guagebar_y1 + Guagebar_y2) / 2)

#define Guagebar_Width    (Guagebar_x2 - Guagebar_x1 + 1)
#define Guagebar_Pos(x)   (Guagebar_Width * (x) / 100 + Guagebar_x1)

#define crGuagebarBkg     GUI_MAKE_COLOR(0x00181818)
#define crGuagebarTop     GUI_MAKE_COLOR(0x00FF4242)
#define crGuagebarMld     GUI_MAKE_COLOR(0x00FFD4C8)
#define crGuagebarBtm     GUI_MAKE_COLOR(0x00DC0000)
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
static uint32_t   FGuagePercent;
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm( uint32_t uCapId )
{
  
  osMutexWait( FGUIState.mutexGUI, osWaitForever );
  
  GUI_SetBkColor( crFrameBkg );  
  GUI_SetColor  ( crFrameBkg );
  GUI_FillRoundedRect( Form_x1, Form_y1, Form_x2, Form_y2, 10 );
  GUI_SetColor  ( crFrameHigh );
  GUI_DrawRoundedFrame( Form_x1, Form_y1, Form_x2, Form_y2, 10, 2 );

  // Caption
  const char* pcCaption = GetMultiLangString( uCapId );
  if( nullptr != pcCaption )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont ( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    
    GUI_RECT Rect = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
    GUI_DispStringInRect( pcCaption, &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
    }

  // Guage Panel
  GUI_SetColor( crSprit );
  GUI_DrawRect( GuagePanel_x1, GuagePanel_y1, GuagePanel_x2, GuagePanel_y2 );
  
  GUI_SetColor( crGuagebarBkg );
  GUI_FillRect( Guagebar_x1, Guagebar_y1, Guagebar_x2, Guagebar_y2 );
  
  osMutexRelease( FGUIState.mutexGUI ); 
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 更新进度条
void GPGuagebar_SetPosition( const uint32_t uPercent )
{
  
  if( FGuagePercent > uPercent )
    {
    // 
    int x1 = Guagebar_Pos(uPercent),
        x2 = Guagebar_Pos(FGuagePercent);
        
    GUI_SetColor( crGuagebarBkg );
    GUI_FillRect( x1, Guagebar_y1, x2, Guagebar_y2 );
    }
  else if( FGuagePercent < uPercent )
    {
    int x1 = Guagebar_Pos(FGuagePercent),
        x2 = Guagebar_Pos(uPercent);
    
    GUI_DrawGradientV( x1, Guagebar_y1, x2, Guagebar_yc, 
                       crGuagebarTop, crGuagebarMld );
    GUI_DrawGradientV( x1, Guagebar_yc, x2, Guagebar_y2, 
                       crGuagebarMld, crGuagebarBtm );
    }
    
  FGuagePercent = uPercent;
}
//-----------------------------------------------------------------------------
// Form接口方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  FGuagePercent = 0;
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{
  
  _FlushForm( (uint32_t)argument );
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg )
{
  
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FGuageForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
