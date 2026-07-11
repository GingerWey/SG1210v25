//-----------------------------------------------------------------------------
/*
 File        : GPTimeDialog.c
 Version     : V1.10
 By          : 银网科技
 Description :时间设置窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GTimeDialog.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "FontCHS24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"

#include "CRC1632.h"
#include "DevRegs.h"
#include "GPVersion.h"

#include "rtc.h"
#include "DevClock.h"
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

#define DIALOG_HEIGHT     128
#define DIALOG_WDITH      (DESKTOP_WIDTH - 30)

#define WIDTH_EDGE        4

#define HEIGHT_INFOITEM   32
//-----------------------------------------------------------------------------
// DateTime
#define DialogEdge_x1   ((DESKTOP_WIDTH  - DIALOG_WDITH)  / 2)
#define DialogEdge_y1   ((DESKTOP_HEIGHT - DIALOG_HEIGHT) / 2)
#define DialogEdge_x2    (DialogEdge_x1 + DIALOG_WDITH  - 1)
#define DialogEdge_y2    (DialogEdge_y1 + DIALOG_HEIGHT - 1)

#define DialogFace_x1    (DialogEdge_x1 + WIDTH_EDGE)
#define DialogFace_y1    (DialogEdge_y1 + WIDTH_EDGE)
#define DialogFace_x2    (DialogEdge_x2 - WIDTH_EDGE)
#define DialogFace_y2    (DialogEdge_y2 - WIDTH_EDGE)

#define Caption_x1       (DialogFace_x1 + WIDTH_EDGE)
#define Caption_y1       (DialogFace_y1 + WIDTH_EDGE)
#define Caption_x2       (DialogFace_x2 - WIDTH_EDGE)
#define Caption_y2       (Caption_y1 + HIGHT_CAPTION - 1)

#define CapLabel_x1      (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1      (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2      (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2      (Caption_y2 - WIDTH_EDGE)

// Editor panel
#define EditorPanel_x1   (CapLabel_x1)
#define EditorPanel_y1   (Caption_y2 + WIDTH_EDGE)
#define EditorPanel_x2   (CapLabel_x2)
#define EditorPanel_y2   (DialogFace_y2 - WIDTH_EDGE - 1)

//
#define Editor_Digtal_Width  12

// Year
#define Editor1_x1       (EditorPanel_x1 + WIDTH_EDGE * 1)
#define Editor1_y1       (EditorPanel_y1 + WIDTH_EDGE * 4)
#define Editor1_x2       (Editor1_x1 + Editor_Digtal_Width * 4 + WIDTH_EDGE * 1)
#define Editor1_y2       (EditorPanel_y2 - WIDTH_EDGE * 4)
// Month
#define Editor2_x1       (Editor1_x2 + WIDTH_EDGE * 2)
#define Editor2_y1       (Editor1_y1)
#define Editor2_x2       (Editor2_x1 + Editor_Digtal_Width * 2 + WIDTH_EDGE * 1)
#define Editor2_y2       (Editor1_y2)
// Day
#define Editor3_x1       (Editor2_x2 + WIDTH_EDGE * 2)
#define Editor3_y1       (Editor1_y1)
#define Editor3_x2       (Editor3_x1 + Editor_Digtal_Width * 2 + WIDTH_EDGE * 1)
#define Editor3_y2       (Editor1_y2)
// Hour
#define Editor4_x1       (Editor3_x2 + WIDTH_EDGE * 4)
#define Editor4_y1       (Editor1_y1)
#define Editor4_x2       (Editor4_x1 + Editor_Digtal_Width * 2 + WIDTH_EDGE * 1)
#define Editor4_y2       (Editor1_y2)
// Minute
#define Editor5_x1       (Editor4_x2 + WIDTH_EDGE * 2)
#define Editor5_y1       (Editor1_y1)
#define Editor5_x2       (Editor5_x1 + Editor_Digtal_Width * 2 + WIDTH_EDGE * 1)
#define Editor5_y2       (Editor1_y2)
// Second
#define Editor6_x1       (Editor5_x2 + WIDTH_EDGE * 2)
#define Editor6_y1       (Editor1_y1)
#define Editor6_x2       (Editor6_x1 + Editor_Digtal_Width * 2 + WIDTH_EDGE * 1)
#define Editor6_y2       (Editor1_y2)

#define crEdiorBkg       GUI_MAKE_COLOR(0x001F1F1F)
#define crEdiorEdge      GUI_MAKE_COLOR(0x002F2F2F)
#define crEdiorDigtal    GUI_WHITE
#define crCursor         GUI_ORANGE    // GUI_BLUE
#define ftEditorDigit    GUI_FONT_24B_ASCII

// Dialog
#define crDialogBkg      GUI_MAKE_COLOR(0x003F1F1F)
#define crDialogFace     GUI_MAKE_COLOR(0x004F2F2F)

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

  TDateTimeType   FdtNow;
  
  const GWinForm *FParent;
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
static const GUI_RECT m_rtTimeEditors[] =
{
  { Editor1_x1, Editor1_y1, Editor1_x2, Editor1_y2 },
  { Editor2_x1, Editor2_y1, Editor2_x2, Editor2_y2 },
  { Editor3_x1, Editor3_y1, Editor3_x2, Editor3_y2 },
  { Editor4_x1, Editor4_y1, Editor4_x2, Editor4_y2 },
  { Editor5_x1, Editor5_y1, Editor5_x2, Editor5_y2 },
  { Editor6_x1, Editor6_y1, Editor6_x2, Editor6_y2 }
};
#define NUM_TimeEditors  (sizeof(m_rtTimeEditors)/sizeof(m_rtTimeEditors[0]))
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
  osMutexWait( FGUIState.mutexGUI, osWaitForever );
  
  GUI_SetDrawMode( GUI_DRAWMODE_NORMAL );
  
  GUI_SetColor( crDialogBkg );
  GUI_RECT Rect1 = { DialogEdge_x1, DialogEdge_y1, DialogEdge_x2, DialogEdge_y2 };
  GUI_FillRectEx( &Rect1 );
  
  GUI_SetColor( crDialogFace );
  GUI_RECT Rect2 = { DialogFace_x1, DialogFace_y1, DialogFace_x2, DialogFace_y2 };
  GUI_FillRectEx( &Rect2 );
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT Rect3 = { CapLabel_x1 + WIDTH_EDGE, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
  GUI_DrawRectEx( &Rect3 );

  const char* pStr = GetMultiLangString( idSetDateTimeCap );
  if( nullptr == pStr )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_DispStringInRect( pStr, &Rect3, GUI_TA_LEFT | GUI_TA_VCENTER );
    }
#endif

  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( EditorPanel_x1, EditorPanel_y1, EditorPanel_x2, EditorPanel_y2 );
  
  GUI_SetColor( crEdiorEdge );
  for( uint32_t uIdx = 0; uIdx < NUM_TimeEditors; uIdx++ )
    {
    const GUI_RECT *rt = m_rtTimeEditors + uIdx;
    GUI_DrawRect( rt->x0 - WIDTH_EDGE, rt->y0 - WIDTH_EDGE,
                  rt->x1 + WIDTH_EDGE, rt->y1 + WIDTH_EDGE );
    }

  osMutexRelease( FGUIState.mutexGUI ); 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _UpdateEditor( uint32_t uIndex )
{

  if( NUM_TimeEditors <= uIndex )
    return ;
  
  GUI_SetColor( crEdiorBkg );
  GUI_FillRectEx( m_rtTimeEditors + uIndex );
    
  if( m_FormState.FEditFiled == uIndex && 1 == (m_FormState.FTick & 1) )
    {
    GUI_SetColor( crCursor );
    GUI_FillRoundedRect( m_rtTimeEditors[uIndex].x1 - Editor_Digtal_Width * (m_FormState.FEditDigNo + 1),
                  m_rtTimeEditors[uIndex].y0 + WIDTH_EDGE,
                  m_rtTimeEditors[uIndex].x1 - Editor_Digtal_Width * (m_FormState.FEditDigNo + 0),
                  m_rtTimeEditors[uIndex].y1 - WIDTH_EDGE, 2 );
    }
    
  char szStr[8];
  szStr[0] = '\x0';
  switch( uIndex )
    {
    case 0:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Year;
        
      sprintf( szStr, "%04u", uValue );
      
      break;
      }
    case 1:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Month;
        
      sprintf( szStr, "%02u", uValue );
      
      break;
      }
    case 2:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Day;
        
      sprintf( szStr, "%02u", uValue );
      
      break;
      }
    case 3:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Hours;
        
      sprintf( szStr, "%02u", uValue );
      
      break;
      }
    case 4:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Minutes;
        
      sprintf( szStr, "%02u", uValue );
      
      break;
      }
    case 5:
      {
      uint32_t uValue;
      if( m_FormState.FEditFiled == uIndex )
        uValue = m_FormState.FEditValue;
      else
        uValue = m_FormState.FdtNow.Seconds;
        
      sprintf( szStr, "%02u", uValue );
      
      break;
      }
    }
    
  if( '\x0' != szStr[0] )
    {
    GUI_SetFont( ftEditorDigit );
    GUI_SetColor( crEdiorDigtal );
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_DispStringInRect( szStr, (GUI_RECT*)(m_rtTimeEditors + uIndex), 
                          GUI_TA_RIGHT | GUI_TA_VCENTER );
    }
}
//-----------------------------------------------------------------------------
static void _UpdateInfoView( uint32_t uAll )
{
  
  if( 0 < uAll )
    {
    for( uint32_t uIdx = 0; uIdx < NUM_TimeEditors; uIdx++ )
      {
      _UpdateEditor( uIdx );
      }
    }
  else
    _UpdateEditor( m_FormState.FEditFiled );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _saveEditorValue()
{
  
  switch( m_FormState.FEditFiled )
    {
    case 0:
      {       
      if( m_FormState.FEditValue < 2000 )
        m_FormState.FEditValue = 2000;
      else if( m_FormState.FEditValue > 2180 )
        m_FormState.FEditValue = 2180;

      m_FormState.FdtNow.Year = m_FormState.FEditValue;
      
      break;
      }
    case 1:
      {
      if( m_FormState.FEditValue < 1 )
        m_FormState.FEditValue = 1;
      else if( m_FormState.FEditValue > 12 )
        m_FormState.FEditValue = 12;

      m_FormState.FdtNow.Month = m_FormState.FEditValue;
      break;
      }
    case 2:
      {
      if( m_FormState.FEditValue < 1 )
        m_FormState.FEditValue = 1;
      else
        {
        uint32_t uDayOfMonth = DEVCLK_DaysOfMonth( m_FormState.FdtNow.Month );
          
        if( 2 == m_FormState.FdtNow.Month && DEVCLK_IsLeapyear( m_FormState.FdtNow.Year ) )
          uDayOfMonth = 29;
          
        if( m_FormState.FEditValue > uDayOfMonth )
          m_FormState.FEditValue = uDayOfMonth;
        }
      
      m_FormState.FdtNow.Day = m_FormState.FEditValue;
        
      break;
      }
    case 3:
      {
      if( m_FormState.FEditValue > 23 )
        m_FormState.FEditValue = 23;

      m_FormState.FdtNow.Hours = m_FormState.FEditValue;
      break;
      }
    case 4:
      {
      if( m_FormState.FEditValue > 59 )
        m_FormState.FEditValue = 59;

      m_FormState.FdtNow.Minutes = m_FormState.FEditValue;
      break;
      }
    case 5:
      {
      if( m_FormState.FEditValue > 59 )
        m_FormState.FEditValue = 59;

      m_FormState.FdtNow.Seconds = m_FormState.FEditValue;
      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _loadEditorValue()
{
  
  switch( m_FormState.FEditFiled )
    {
    case 0:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Year;
      break;
      }
    case 1:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Month;
      break;
      }
    case 2:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Day;
      break;
      }
    case 3:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Hours;
      break;
      }
    case 4:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Minutes;
      break;
      }
    case 5:
      {
      m_FormState.FEditValue = m_FormState.FdtNow.Seconds;
      break;
      }
    }
}
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
    _saveEditorValue();

    if( NUM_TimeEditors > m_FormState.FEditFiled + 1 )
      {
      m_FormState.FEditDigNo = 1;  
      m_FormState.FEditFiled++;
      }
    else
      { // 卷回
      m_FormState.FEditDigNo = 1;  // 只让编辑年后2位
      m_FormState.FEditFiled = 0;
      }

    _loadEditorValue();
      
    _UpdateEditor( uOldField );      
    _UpdateEditor( m_FormState.FEditFiled );
    }
}
//-----------------------------------------------------------------------------
static void _shiftToLeft()
{

  uint32_t uOldField = m_FormState.FEditFiled;
  if( 0 == m_FormState.FEditFiled )
    {
    if( m_FormState.FEditDigNo < 1 )  // 只让编辑年后2位
      {
      m_FormState.FEditDigNo++;
        
      _UpdateEditor( m_FormState.FEditFiled );
      }
    else
      { // 卷回
      _saveEditorValue();
        
      m_FormState.FEditFiled = NUM_TimeEditors - 1;
      m_FormState.FEditDigNo = 0;
 
      _loadEditorValue();

      _UpdateEditor( uOldField );
      _UpdateEditor( m_FormState.FEditFiled );
      }
    }
  else if( m_FormState.FEditDigNo < 1)
    {
    m_FormState.FEditDigNo++;
        
    _UpdateEditor( m_FormState.FEditFiled );
    }
  else 
    {
    _saveEditorValue();

    m_FormState.FEditDigNo = 0;  
    m_FormState.FEditFiled--;

    _loadEditorValue();

    _UpdateEditor( uOldField );
    _UpdateEditor( m_FormState.FEditFiled );
    }     
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_FormState, 0, sizeof(m_FormState) );
  
  m_FormState.FParent = (const GWinForm*)argument;

  // 
  RTC_GetTime( &m_FormState.FdtNow );
  
  m_FormState.FEditValue = m_FormState.FdtNow.Year;
  m_FormState.FEditFiled = 0;
  m_FormState.FEditDigNo = 0;  // 只让编辑年后2位
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
      _saveEditorValue();
        
      m_FormState.FdtNow.MilSeconds = 0;
      //DEVCLK_SetDateTime( &DevClock, &(m_FormState.FdtNow) );
      RTC_SetTime( &m_FormState.FdtNow );
        
      if( 0 != m_FormState.FParent )
        {
        GM_MESSAGE aMsg = { GM_EDITOR_ACCEPT, 0 };
        m_FormState.FParent->pMsg( &aMsg );
        }
      
      break;
      }

    case KEY_ESCAPE:
      {
      if( 0 != m_FormState.FParent )
        {
        GM_MESSAGE aMsg = { GM_EDITOR_CANCEL, 0 };
        m_FormState.FParent->pMsg( &aMsg );
        }

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
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FTimeDialog = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
