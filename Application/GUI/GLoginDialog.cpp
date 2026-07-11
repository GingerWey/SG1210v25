//-----------------------------------------------------------------------------
/*
 File        : GPLoginDialog.c
 Version     : V1.10
 By          : 银网科技
 Description : 口令登录窗体
        
 Date        : 2024.4.6
*/
//-----------------------------------------------------------------------------
#include "GLoginDialog.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "FontCHS24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"
#include "GMessageForm.h"

#include "DevFixed.h"

#include "Strings/TextStrs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32f4xx_hal.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define  GetRegisterValue(x)      DevConfig.Configs[(x) - REG_DEVCONFIG]
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     40

#define WIDTH_EDGE        4

#define HIGHT_EDITOR      40
//-----------------------------------------------------------------------------
// Icon
#define ICON_HEIGHT       80
#define ICON_WIDTH        100

#define Icon_x1          (WIDTH_EDGE)
#define Icon_y1          ((DESKTOP_HEIGHT - ICON_HEIGHT) / 2)
#define Icon_x2          (Icon_x1 + ICON_WIDTH)
#define Icon_y2          (Icon_y1 + ICON_HEIGHT)

// Caption
#define Caption_x1       (Icon_x2 + WIDTH_EDGE)
#define Caption_y1      ((DESKTOP_HEIGHT / 2) - HIGHT_CAPTION - 2 * WIDTH_EDGE)
#define Caption_x2       (DESKTOP_WIDTH - WIDTH_EDGE)
#define Caption_y2      ((DESKTOP_HEIGHT / 2) - WIDTH_EDGE)

#define CapLabel_x1      (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1      (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2      (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2      (Caption_y2 - WIDTH_EDGE)

// Editor panel
#define EditorPanel_x1   (Caption_x1)
#define EditorPanel_y1  ((DESKTOP_HEIGHT / 2) + WIDTH_EDGE)
#define EditorPanel_x2   (Caption_x2)
#define EditorPanel_y2   (EditorPanel_y1 + HIGHT_EDITOR + WIDTH_EDGE - 1)

//
#define Editor_Digtal_Width  15

// Password
#define Editor1_x1       ((EditorPanel_x1 + EditorPanel_x2) / 2 - \
                          Editor_Digtal_Width * 2 - WIDTH_EDGE)
#define Editor1_y1       (EditorPanel_y1 + WIDTH_EDGE)
#define Editor1_x2       (Editor1_x1 + Editor_Digtal_Width * 4 + WIDTH_EDGE)
#define Editor1_y2       (EditorPanel_y2 - WIDTH_EDGE)

#define crEdiorBkg       GUI_DARKBLUE
#define crEdiorEdge      GUI_MAKE_COLOR(0x002F2F2F)
#define crEdiorDigtal    GUI_WHITE
#define crCursor         GUI_ORANGE    // GUI_BLUE
#define ftEditorDigit    &GUI_Font32B_ASCII

// Colors
#define crFormBkg        GUI_MAKE_COLOR(0x001F1F1F)
#define crFrameHigh      GUI_MAKE_COLOR(0x002F2F2F)
#define crFrameDrak      GUI_MAKE_COLOR(0x000F0F0F)

#define crCaptionFont    GUI_GRAY_9A  
#define ftCaption        GUI_FONT_24LTH_CHN

#define crInfoBkg        GUI_MAKE_COLOR(0x00181818)

#define crInfoText       GUI_ORANGE
#define ftInfoText       GUI_FONT_24LTH_CHN
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 状态数据
typedef struct tagGFormState
{

  uint8_t     FTopItem;
  uint8_t     FCurItem;
  uint16_t    FCount;
  
  uint16_t    FTick;
  uint32_t    FNextTick;

  uint8_t     FEditFiled;
  uint8_t     FEditDigNo;
  
  uint16_t    FEditValue;

  const GWinForm *FDialog;
} GFormState;
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 列表状态
static GFormState m_FormState = {0};
//-----------------------------------------------------------------------------
// 各位值
static const uint32_t FPow10[] =
{ 1, 10, 100, 1000 };
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//static const GUI_RECT m_rtTimeEditors[] =
//{
//  { Editor1_x1, Editor1_y1, Editor1_x2, Editor1_y2 },
//};
//#define NUM_TimeEditors  (sizeof(m_rtTimeEditors)/sizeof(m_rtTimeEditors[0]))
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
  osMutexWait( FGUIState.mutexGUI, osWaitForever );
  
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

  GUI_SetDrawMode( GUI_DRAWMODE_NORMAL );
  
  GUI_SetColor( crFrameHigh );
  GUI_FillRect( 0, DESKTOP_WIDTH - 1, 0, DESKTOP_HEIGHT - 1 );

#if ICON_HEIGHT > 0
  GUI_DrawPicture( &picKey99x77G32jpg, Icon_x1, Icon_y1 );
#endif
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT rtCap = { Caption_x1, Caption_y1, 
                     Caption_x2, Caption_y2 };
//  GUI_DrawRectEx( &rtCap );

  const char* pStr = GetMultiLangString( idLoginCap1 );
  if( nullptr != pStr )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    
    rtCap.x0 += WIDTH_EDGE * 4;
    GUI_DispStringInRect( pStr, &rtCap, GUI_TA_LEFT | GUI_TA_VCENTER );
    }
#endif

//  GUI_SetColor( crFrameHigh );
//  GUI_DrawRect( EditorPanel_x1, EditorPanel_y1, 
//                EditorPanel_x2, EditorPanel_y2 );
  
  GUI_SetColor( crEdiorEdge );
#if NUM_TimeEditors > 0
  for( uint32_t uIdx = 0; uIdx < NUM_TimeEditors; uIdx++ )
    {
    const GUI_RECT *rt = m_rtTimeEditors + uIdx;
    GUI_DrawRect( rt->x0 - WIDTH_EDGE, rt->y0 - WIDTH_EDGE,
                  rt->x1 + WIDTH_EDGE, rt->y1 + WIDTH_EDGE );
#else
    {
    GUI_DrawRect( Editor1_x1, Editor1_y1, Editor1_x2, Editor1_y2 );
    }
#endif
    
  osMutexRelease( FGUIState.mutexGUI ); 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _UpdateEditor( uint32_t uIndex )
{

//  if( NUM_TimeEditors <= uIndex )
//    return ;
  
  GUI_SetColor( crEdiorBkg );
  GUI_FillRect( Editor1_x1, Editor1_y1, Editor1_x2, Editor1_y2 );
    
  if( //m_FormState.FEditFiled == uIndex && 
      1 == (m_FormState.FTick & 1) )
    {
    GUI_SetColor( crCursor );
      
    #define xCursor (Editor1_x2 - Editor_Digtal_Width * m_FormState.FEditDigNo)
    GUI_FillRoundedRect( xCursor - Editor_Digtal_Width + 1,
                         Editor1_y1 + WIDTH_EDGE,
                         xCursor,
                         Editor1_y2 - WIDTH_EDGE, 2 );
    }
    
  char szStr[6];
  sprintf( szStr, "%04u", m_FormState.FEditValue );

  GUI_SetFont ( ftEditorDigit );
  GUI_SetColor( crEdiorDigtal );
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    
  GUI_RECT rtText = { Editor1_x1, Editor1_y1, Editor1_x2, Editor1_y2 };
  GUI_DispStringInRect( szStr, &rtText, 
                        GUI_TA_RIGHT | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _UpdateInfoView( uint32_t uAll )
{
  
//  if( 0 < uAll )
//    {
    _UpdateEditor( 0 );
//    }
//  else
//    _UpdateEditor( m_FormState.FEditFiled );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _shiftToRight()
{

  uint32_t uOldField = m_FormState.FEditFiled;
  if( m_FormState.FEditDigNo > 0 )
    {
    m_FormState.FEditDigNo--;
    
    _UpdateEditor( m_FormState.FEditFiled );
    }
  else
    {
    m_FormState.FEditDigNo = 3;
    
    _UpdateEditor( m_FormState.FEditFiled );
    }
}
//-----------------------------------------------------------------------------
static void _shiftToLeft()
{

  if( m_FormState.FEditDigNo < 3 )
    {
    m_FormState.FEditDigNo++;
      
    _UpdateEditor( m_FormState.FEditFiled );
    }
  else
    { // 卷回
    m_FormState.FEditDigNo = 0;
    }
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_FormState, 0, sizeof(m_FormState) );
  
  m_FormState.FEditValue = 0;
  m_FormState.FEditFiled = 0;
  m_FormState.FEditDigNo = 3;
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{
  
  _FlushForm();
  
  _UpdateInfoView( 1 );
  
  // Next Tick
  m_FormState.FNextTick = GUI_GetTime() + 500;
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
    m_FormState.FNextTick =  uTick+ 500;
    
    m_FormState.FTick++;
      
    _UpdateEditor( m_FormState.FEditFiled );
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{

  switch( uwKey )
    {
    case KEY_RIGHT:
      {
      _shiftToRight();
        
      break;
      }

    case KEY_LEFT:
      {
      _shiftToLeft();

      break;
      }
      
    case KEY_ADD:
    case KEY_UP:
    case KEY_SUB:
    case KEY_DOWN:
      {
      uint32_t uPow10 = FPow10[m_FormState.FEditDigNo],
               uValue = m_FormState.FEditValue;
        
      uint32_t uDigit = ((uValue / uPow10)  % 10);
      uValue -= uDigit * uPow10;
        
      if( KEY_ADD == uwKey || KEY_UP == uwKey )
        {
        if( uDigit < 9 )
          uDigit++;
        else
          uDigit = 0;
        }
      else
        {
        if( uDigit > 0 )
          uDigit--;
        else
          uDigit = 9;
        }
      
      uValue += uDigit * uPow10;
        
      m_FormState.FEditValue = uValue;
      _UpdateInfoView( 0 );
      
      break;
      }
      
    case KEY_ENTER:
      {
      if( m_FormState.FEditValue == DevConfig.Password )
        {
        GUISendMessage( GM_EDITOR_ACCEPT, 
                        m_FormState.FEditValue, 
                        (uint32_t)&FLoginDialog );
        }
      else
        {
        GMessageType aMsg;
        aMsg.FType    = GMT_ALARM;
        aMsg.FMessage = GetMultiLangString(idLoginChkErrTxt);
      
        m_FormState.FDialog = &FMessageForm;
        FMessageForm.pInit( &aMsg );
        FMessageForm.pShow( &aMsg );
        }
      
      break;
      }

    case KEY_ESCAPE:
      {
      GUISendMessage( GM_EDITOR_CANCEL, 0, (uint32_t)&FLoginDialog );

      break;
      }

    default:
      {
      if( uwKey >= '0' &&  uwKey <= '9' )
        {
        uint32_t uPow10 = FPow10[m_FormState.FEditDigNo],
                 uValue = m_FormState.FEditValue;
          
        uValue -= ((uValue / uPow10)  % 10) * uPow10;
        uValue += (uwKey - '0') * uPow10;
          
        m_FormState.FEditValue = uValue;
          
        _shiftToRight();
        }
      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg)
{
  
  if( 0 == pMsg )
    return ;
  
  if( nullptr != m_FormState.FDialog )
    m_FormState.FDialog->pMsg( pMsg );
  
  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      { 
      _OnTick( pMsg->Data.v );

      pMsg->MsgId = 0;    // accepted
      break;
      }

    case GM_KEYDOWN:
      {
      if( pMsg->Param )
        {
        _OnKeyDown( pMsg->Param );

        pMsg->MsgId = 0;  // accepted
        }

      break;
      }

    case GM_EDITOR_ACCEPT:
      {
      if( nullptr != m_FormState.FDialog &&
          &FMessageForm == pMsg->Data.p )
        {
        m_FormState.FDialog->pClose( nullptr );
        m_FormState.FDialog = nullptr;

        // 重绘窗体
        _Show( nullptr );

        pMsg->MsgId = 0;
        }

      break;
      }
      
    case GM_EDITOR_CANCEL:
      {
      if( nullptr != m_FormState.FDialog )
        {
        m_FormState.FDialog->pClose( nullptr );
        m_FormState.FDialog = nullptr;
        }

      GUISendMessage( GM_EDITOR_CANCEL, 
                      0, 
                      (uint32_t)&FLoginDialog );

      pMsg->MsgId = 0;
        
      break;
      }
      
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FLoginDialog = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
