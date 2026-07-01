//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPSplashForm.cpp
 Version     : V2.00
 By          : Wey. Silver Grid

 Description : Splash form. Displays device branding, auto-transitions to
               GMainForm after timeout or user interaction.
               Per SG1210v21 form design spec section I.

   Layer0: Full-screen background (picbkg320x240Lcsg)
   Layer1: Device model, Chinese/English names, copyright overlay

   Behaviour:
     - Created by GUIStart on power-up, or via ESC from GMainForm
     - Auto-exits to GMainForm after 60s idle
     - Any key press or touch click immediately exits

 Date        : 2026.06.29 (V2.00 - rewritten per form design spec)
              2026.06.26 (V1.15 - CSG decoder test rig, removed)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "GPSplashForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

#include "FontSGRes.h"
#include "GUIPicture.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"
#include "GUIMisc.h"

#include <string.h>
#include <GUI_Type.h>

//=============================================================================
// Layout constants - per spec section I.1.2
// Coordinate format (x, y, w, h), top-left origin.
// Converted to emWin (x0,y0,x1,y1) via: x1=x+w-1, y1=y+h-1
//=============================================================================

// Background
#define SP_BKG_X          0
#define SP_BKG_Y          0

// Device model label - e.g. "SG1210"
#define SP_MODEL_X         10
#define SP_MODEL_Y         64
#define SP_MODEL_W         300
#define SP_MODEL_H         24
#define SP_MODEL_FONT      GUI_FONT_24LTH_CHN
#define SP_MODEL_COLOR     0x00B5F2
#define SP_MODEL_ALIGN     (GUI_TA_HCENTER | GUI_TA_VCENTER)
#define SP_MODEL_TEXT      idGPDevFamiry

// Device function name - Chinese (multi-lang)
#define SP_NAME_X          10
#define SP_NAME_Y          102
#define SP_NAME_W          300
#define SP_NAME_H          24
#define SP_NAME_FONT       GUI_FONT_24LTH_CHN
#define SP_NAME_COLOR      0x00B5F2
#define SP_NAME_ALIGN      (GUI_TA_HCENTER | GUI_TA_VCENTER)
#define SP_NAME_TEXT       idDevFuncUVTC

// Device function name - English (static)
#define SP_ENG_X           10
#define SP_ENG_Y           128
#define SP_ENG_W           300
#define SP_ENG_H           17
#define SP_ENG_FONT        GUI_FONT_AA4_ASCII16B
#define SP_ENG_COLOR       0x00B5F2
#define SP_ENG_ALIGN       (GUI_TA_HCENTER | GUI_TA_VCENTER)
#define SP_ENG_TEXT        "Undervoltage Trip Controller"

// Copyright line
#define SP_COPY_X          10
#define SP_COPY_Y          219
#define SP_COPY_W          300
#define SP_COPY_H          16
#define SP_COPY_FONT       GUI_FONT_AA4_ASCII16B
#define SP_COPY_COLOR      0x00B5F2
#define SP_COPY_ALIGN      (GUI_TA_HCENTER | GUI_TA_VCENTER)

// Timeout
#define SP_TIMEOUT_MS      30000     // 30 seconds idle timeout

//=============================================================================
// Form state
//=============================================================================
typedef struct tagSplashState {
  uint32_t uStartTime;    // GUI_GetTime() at form entry (ms)
  bool     bTimedOut;     // True once timeout has fired
} TSplashState;

static TSplashState m_State;

//=============================================================================
// Helper: convert (x,y,w,h) to emWin GUI_RECT (inclusive x0,y0,x1,y1)
//=============================================================================
static void _MakeRect(GUI_RECT* pR, int x, int y, int w, int h)
{
  pR->x0 = x;
  pR->y0 = y;
  pR->x1 = x + w - 1;
  pR->y1 = y + h - 1;
}

//=============================================================================
// Drawing
//=============================================================================

//-----------------------------------------------------------------------------
// Draw Layer0 full-screen background
//-----------------------------------------------------------------------------
static void _DrawBackground(void)
{

  GUI_DrawPicture(&picbkg320x240Lcsg, SP_BKG_X, SP_BKG_Y, 0);
}

//-----------------------------------------------------------------------------
// Draw a multi-lang text label. Skips silently if string is nullptr.
//-----------------------------------------------------------------------------
static void _DrawMLabel(uint32_t uTextId, const GUI_FONT* pFont,
                        int x, int y, int w, int h,
                        GUI_COLOR color, int align)
{
  const char* pStr = GetMultiLangString(uTextId);
  if (nullptr == pStr) return;

  GUI_RECT rect;
  _MakeRect(&rect, x, y, w, h);

  GUI_SetFont(pFont);
  GUI_SetColor(color);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &rect, align);
}

//-----------------------------------------------------------------------------
// Draw a static text label
//-----------------------------------------------------------------------------
static void _DrawSLabel(const char* pStr, const GUI_FONT* pFont,
                        int x, int y, int w, int h,
                        GUI_COLOR color, int align)
{
  if (nullptr == pStr) return;

  GUI_RECT rect;
  _MakeRect(&rect, x, y, w, h);

  GUI_SetFont(pFont);
  GUI_SetColor(color);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &rect, align);
}

//-----------------------------------------------------------------------------
// Draw all Layer1 overlay text elements
//-----------------------------------------------------------------------------
static void _DrawOverlay(void)
{
  // Device model - via multi-lang string
  _DrawMLabel(SP_MODEL_TEXT, SP_MODEL_FONT,
              SP_MODEL_X, SP_MODEL_Y, SP_MODEL_W, SP_MODEL_H,
              SP_MODEL_COLOR, SP_MODEL_ALIGN);

  // Device function name (Chinese) - via multi-lang string
  _DrawMLabel(SP_NAME_TEXT, SP_NAME_FONT,
              SP_NAME_X, SP_NAME_Y, SP_NAME_W, SP_NAME_H,
              SP_NAME_COLOR, SP_NAME_ALIGN);

  // Device function name (English) - static string
  _DrawSLabel(SP_ENG_TEXT, SP_ENG_FONT,
              SP_ENG_X, SP_ENG_Y, SP_ENG_W, SP_ENG_H,
              SP_ENG_COLOR, SP_ENG_ALIGN);

  // Copyright - via multi-lang string
  _DrawMLabel(idDevCopyright, SP_COPY_FONT,
              SP_COPY_X, SP_COPY_Y, SP_COPY_W, SP_COPY_H,
              SP_COPY_COLOR, SP_COPY_ALIGN);
}

//-----------------------------------------------------------------------------
// Full screen redraw: clear, background, then overlay
//-----------------------------------------------------------------------------
static void _Redraw(void)
{
  
  GUI_SetBkColor(GUI_BLACK);
  GUI_Clear();

  _DrawBackground();

  _DrawOverlay();

  //GUI_DrawPicture(&picMAUAtlascsg,  10, 10, picIdxMU_Item32x32_01, 100);
  //GUI_DrawPicture(&picMAUAtlascsg,  60, 10, picIdxMU_Item32x32_02, 100);
  //GUI_DrawPicture(&picMAUAtlascsg, 110, 10, picIdxMU_Item32x32_03, 100);
  //GUI_DrawPicture(&picMAUAtlascsg, 160, 10, picIdxMU_Item32x32_04, 100);
  //GUI_DrawPicture(&picMAUAtlascsg, 210, 10, picIdxMU_Item32x32_05, 100);
  //GUI_DrawPicture(&picMAUAtlascsg, 260, 10, picIdxMU_Item32x32_06, 100);
}

//=============================================================================
// Navigation
//=============================================================================
static void _GoToMain(void)
{
  gfc::ReplaceForm(WID_MainForm, nullptr);
}

//=============================================================================
// Form lifecycle callbacks
//=============================================================================

//-----------------------------------------------------------------------------
// _Init: reset state. Called once when form is first created.
//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{
  (void)argument;
  memset(&m_State, 0, sizeof(m_State));
}

//-----------------------------------------------------------------------------
// _Show: called each time the form becomes visible.
// Reset the timeout timer and redraw.
//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{
  (void)argument;
  m_State.uStartTime = GUI_GetTime();
  m_State.bTimedOut  = false;
  _Redraw();
}

//-----------------------------------------------------------------------------
// _Close: called when form is removed from screen.
// Nothing to free (static allocation).
//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
  (void)argument;
}

//-----------------------------------------------------------------------------
// _OnTick: periodic timer. Checks 60-second idle timeout.
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
  (void)uTick;
  if (m_State.bTimedOut) return;

  if (GUI_GetTime() - m_State.uStartTime >= SP_TIMEOUT_MS)
  {
    m_State.bTimedOut = true;
    _GoToMain();
  }
}

//-----------------------------------------------------------------------------
// _OnKeyUp: any key release immediately exits to GMainForm (spec I.1.1).
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{
  (void)uwKey;
  _GoToMain();
}

//-----------------------------------------------------------------------------
// _OnTouch: touch-up immediately exits to GMainForm (spec I.1.1).
//-----------------------------------------------------------------------------
static void _OnTouch(uint16_t action)
{
  if (action == TOUCH_UP)
  {
    _GoToMain();
  }
}

//-----------------------------------------------------------------------------
// _OnMessage: central message dispatcher.
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE* pMsg)
{
  if (nullptr == pMsg) return;

  switch (pMsg->MsgId)
  {

  case GM_TIMER_TICK:
    _OnTick(static_cast<uint32_t>(pMsg->Data.v));
    break;

  case GM_KEYUP:
    if (pMsg->Param)
    {
      _OnKeyUp(pMsg->Param);
      pMsg->MsgId = 0;
    }
    break;

#if GUI_SUPPORT_TOUCH
  case GM_TOUCH:
    _OnTouch(pMsg->Param);
    pMsg->MsgId = 0;
    break;
#endif

  default:
    break;
  }
}

//=============================================================================
// Form descriptor - exposed via GPSplashForm.h
//=============================================================================
const GWinForm FSplashForm =
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// Auto-register with GFormCentra navigation system
static const gfc::FormRegistrar kRegSplash(WID_SplashForm, &FSplashForm, "Splash");
//-----------------------------------------------------------------------------
