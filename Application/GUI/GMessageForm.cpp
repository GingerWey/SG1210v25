//-----------------------------------------------------------------------------
/*
 File        : GPMessageForm.c
 Version     : V1.10
 By          : 银网科技
 Description :消息窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GMessageForm.h"

#include "GUI.h"
#include "GUIConf.h"
#include "GUICntr.h"
//#include "FontCHS16LTH.h"
#include "FontCHS24LTH.h"

#include "GUIPicture.h"
#include "PictureRes.h"
#include "Strings/TextStrs.h"

#include "DevRegs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     48

#define HEIGHT_BUTTON     50

#define WIDTH_EDGE        4
//-----------------------------------------------------------------------------
#define Caption_x1        (0)
#define Caption_y1        (0)
#define Caption_x2        (DESKTOP_WIDTH - 1)
#define Caption_y2        (Caption_y1 + HIGHT_CAPTION - 1)

#define CapLabel_x1       (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1       (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2       (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2       (Caption_y1 + HIGHT_CAPTION - 1)

// Info panel
#define InfoPanel_x1      (0 + WIDTH_EDGE)
#define InfoPanel_y1      (Caption_y2 + WIDTH_EDGE)
#define InfoPanel_x2      (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define InfoPanel_y2      (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define InfoView_x1       (InfoPanel_x1 + WIDTH_EDGE)
#define InfoView_y1       (InfoPanel_y1 + WIDTH_EDGE)
#define InfoView_x2       (InfoPanel_x2 - WIDTH_EDGE)
#define InfoView_y2       (InfoPanel_y2 - HEIGHT_BUTTON - WIDTH_EDGE)

#define InfoView_W        (InfoView_x2 - InfoView_x1 + 1)
#define InfoView_H        (InfoView_y2 - InfoView_y1 + 1)

// ICON
#define Icon_x1           (InfoView_x1 + WIDTH_EDGE )
#define Icon_y1           (InfoView_y1 + (InfoView_H - 64) / 2)
#define Icon_x2           (Icon_x1     + 64 )
#define Icon_y2           (Icon_y1     + 64 )

// MsgInfo
#define MsgInfo_x1        (Icon_x2     + WIDTH_EDGE )
#define MsgInfo_y1        (InfoView_y1 + WIDTH_EDGE )
#define MsgInfo_x2        (InfoView_x2 - WIDTH_EDGE )
#define MsgInfo_y2        (InfoView_y2 - WIDTH_EDGE )

// Button
#define WIDTH_BUTTON      120
#define Button_x1         ((DESKTOP_WIDTH - WIDTH_BUTTON) / 2)
#define Button_y1         (Button_y2    - HEIGHT_BUTTON)
#define Button_x2         (Button_x1    + WIDTH_BUTTON)
#define Button_y2         (InfoPanel_y2 - WIDTH_EDGE)

// Colors
#define crFormBkg         GUI_MAKE_COLOR(0x001F1F1F)  
#define crFrameHigh       GUI_MAKE_COLOR(0x002F2F2F)  
#define crFrameDrak       GUI_MAKE_COLOR(0x000F0F0F)

#define crCaptionFont     GUI_GRAY_9A  
#define ftCaption         GUI_FONT_24LTH_CHN

#define ftMsgInfo         GUI_FONT_24LTH_CHN

#define crInfoBkg         GUI_MAKE_COLOR(0x00181818) 

// Button
#define crButtonEdge      GUI_LIGHTCYAN
#define crButtonFace      GUI_GRAY_3F
#define crButtonFont      GUI_WHITE
#define ftButtonCap       GUI_FONT_24LTH_CHN
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
typedef struct tagMessageStyle
{
  const TGUIPicture *pIcon;
  uint32_t           TextColor;
} GMessageStyle;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
const GMessageStyle FMsgStyles[] = 
{
   {&picInfoMark64x64C565bmp,  GUI_WHITE       }
  ,{&picQuestMark64x64C565bmp, GUI_LIGHTCYAN   }
  ,{&picExclMark63x56C565bmp,  GUI_YELLOW      }
  ,{&picStopMark64x64C565bmp,  GUI_RED         }
};
#define NUM_MessageStyles  (sizeof(FMsgStyles) / sizeof(FMsgStyles[0]))
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
  osMutexWait( FGUIState.mutexGUI, osWaitForever );
  
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT Rect1 = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
  GUI_DrawRectEx( &Rect1 );

  const char* pStr = GetMultiLangString( idDevFuncUVTC );
  if( nullptr != pStr )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    
    GUI_DispStringInRect( pStr, &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
    }
#endif
  
  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( InfoPanel_x1, InfoPanel_y1, InfoPanel_x2, InfoPanel_y2 );
  
  // Button
  GUI_SetColor( crButtonEdge );
  GUI_DrawRect( Button_x1, Button_y1, Button_x2, Button_y2 );
  
  Rect1.x0 = Button_x1 + WIDTH_EDGE;
  Rect1.y0 = Button_y1 + WIDTH_EDGE;
  Rect1.x1 = Button_x2 - WIDTH_EDGE;
  Rect1.y1 = Button_y2 - WIDTH_EDGE;
  GUI_SetColor( crButtonFace );
  GUI_FillRectEx( &Rect1 );
  
  pStr = GetMultiLangString( idBtnCapClose );
  if( nullptr != pStr )
    {
    GUI_SetColor( crButtonFont );
    GUI_SetFont ( ftButtonCap  ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_DispStringInRect( pStr, &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
    }

  osMutexRelease( FGUIState.mutexGUI ); 
}
//-----------------------------------------------------------------------------
static void _ShowMessage( const GMessageType* pMsg )
{
  
  if( nullptr == pMsg || nullptr == pMsg->FMessage || 
      NUM_MessageStyles <= pMsg->FType )
    return ;
  
  GUI_DrawPicture( FMsgStyles[pMsg->FType].pIcon, Icon_x1, Icon_y1 );

  GUI_SetColor( FMsgStyles[pMsg->FType].TextColor );
  GUI_SetFont ( ftMsgInfo ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
  GUI_RECT Rect = { MsgInfo_x1, MsgInfo_y1, MsgInfo_x2, MsgInfo_y2 };
  GUI_DispStringInRect( pMsg->FMessage, &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
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
  
  _FlushForm();
  
  _ShowMessage( (const GMessageType*)argument );
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
static void _OnKeyDown( uint16_t uwKey )
{

  if( KEY_ESCAPE == uwKey || KEY_ENTER == uwKey )
    {
    GUISendMessage( GM_EDITOR_ACCEPT, 
                    (KEY_ENTER == uwKey), 
                    (uint32_t)&FMessageForm );
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg )
{
  
  if( 0 == pMsg )
    return ;
  
  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      { 
      _OnTick( pMsg->Data.v );

      pMsg->MsgId = 0;

      break;
      }
      
    case GM_KEYDOWN:
      {
      _OnKeyDown( pMsg->Param );

      pMsg->MsgId = 0;

      break;
      }
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FMessageForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
