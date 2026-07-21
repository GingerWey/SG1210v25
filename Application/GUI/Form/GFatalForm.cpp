//-----------------------------------------------------------------------------
/*
 File        : GFatalForm.h
 Version     : V1.10
 By          : 银网科技
 Description : 出错窗体
        
 Date        : 2025.8.18
*/
//-----------------------------------------------------------------------------
#include "GFatalForm.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "GUIMisc.h"

#include "FontCHS16LTH.h"
#include "FontCHS24LTH.h"

#include "Strings/TextStrs.h"

//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION       32

#define WIDTH_EDGE          6

//-----------------------------------------------------------------------------
#define crFormBkg           GUI_MAKE_COLOR(0x00A00000)
#define crFrameHigh         GUI_WHITE  
#define crFrameDrak         GUI_MAKE_COLOR(0x00400000)

// Caption
#define crCaptionBkg        GUI_BLUE
#define crCaptionFont       GUI_LIGHTRED
#define ftCaptionText       GUI_FONT_16LTH_CHN

#define Caption_x1          (0)
#define Caption_y1          (0)
#define Caption_x2          (DESKTOP_WIDTH  - 1)
#define Caption_y2          (HIGHT_CAPTION - 1)

#define CapLabel_x1         (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1         (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2         (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2         (Caption_y2 - 1)

// Infos
#define crFatalInfoBkg      GUI_BLUE
#define crFatalInfoFont     GUI_WHITE
#define ftFatalInfoText     GUI_FONT_16LTH_CHN

#define rtStateInfo_x1      (WIDTH_EDGE)
#define rtStateInfo_y1      (CapLabel_y2 + WIDTH_EDGE * 2)
#define rtStateInfo_x2      (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define rtStateInfo_y2      (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define rtFatalInfo_x1      (rtStateInfo_x1 + WIDTH_EDGE * 2)
#define rtFatalInfo_y1      (rtStateInfo_y1 + WIDTH_EDGE)
#define rtFatalInfo_x2      (rtStateInfo_x2 - WIDTH_EDGE * 2)
#define rtFatalInfo_y2      (rtStateInfo_y2 - WIDTH_EDGE)

//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _showCaption()
{

  // Caption
  GUI_SetColor( crCaptionBkg );
  GUI_FillRect( Caption_x1, Caption_y1, Caption_x2, Caption_y2 );
  
  GUI_SetFont ( ftCaptionText );
  GUI_SetColor( crCaptionFont );
  
  // 窗标题
  GUI_RECT aRect = { Caption_x1 + WIDTH_EDGE, Caption_y1 + WIDTH_EDGE, 
                     Caption_x2 - WIDTH_EDGE, Caption_y2 - WIDTH_EDGE };
  
  const char* szCap = GetMultiLangString( iaCriticalInfo );
  GUI_DispStringInRect(  szCap, 
                        &aRect, 
                         GUI_TA_CENTER | GUI_TA_VCENTER );
}

//-----------------------------------------------------------------------------
static void _showInfo( const char* pcMsg )
{

  // Infos
  GUI_SetColor( crFrameDrak );
  GUI_DrawRect( rtStateInfo_x1, rtStateInfo_y1, rtStateInfo_x2, rtStateInfo_y2 );

  GUI_RECT aRect = { rtFatalInfo_x1, rtFatalInfo_y1, 
                     rtFatalInfo_x2, rtFatalInfo_y2 };

  GUI_SetFont ( ftFatalInfoText );
  GUI_SetColor( crFatalInfoFont );
  GUI_DispStringInRect(  pcMsg, 
                        &aRect, 
                         GUI_TA_TOP | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _showForm( const char* pcMsg )
{

  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

  GUI_SetTextMode( GUI_TEXTMODE_TRANS );

  _showCaption();
  
  _showInfo( pcMsg );
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{

  const char* pcMsg = (const char*)argument;
  
  if( nullptr != pcMsg )
    _showForm( pcMsg );
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{
  
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
}
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE* pMsg)
{
  
  if( 0 == pMsg )
    return ;
  
  switch( pMsg->MsgId )
    {
//    case GM_TIMER_TICK:
//      { 
//      _OnTick( pMsg->Data.v );
//        
//      break;
//      }
//      
//    case GM_KEYDOWN:
//      {
//      if( pMsg->Param )
//        {
//        _OnKeyDown( pMsg->Param );
//          
//        pMsg->MsgId = 0;
//        }
//        
//      break;
//      }
//      
//    case GM_TIMERECV:
//      {
//      if( FGUIState.pCurForm == &FMainForm )
//        _UpdateClock();
//      
//      break;
//      }
      
    case GM_DATARECV:
      {
      
      break;
      }
    }
}

//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FFatalForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
