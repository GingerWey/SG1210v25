//-----------------------------------------------------------------------------
/*
 File        : GPLogListForm.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : LogList form — log browser for event/alarm/fault logs.
               Displays log entries grouped by type with caption.
               Supports touch swipe and keyboard navigation.
               Per SG1210v25 form design spec section V.

 Date        : 2026.07.10 (V1.00 — initial implementation)
*/
//-----------------------------------------------------------------------------
#include "GLogListForm.h"

#include "GUI.h"
#include "GFormCentra.h"tio
#include "GUICntr.h"
#include "GUIConf.h"
#include "FontCHS16LTH.h"

#include "DevFixed.h"
#include "DevEvtMgr.h"
#include "DevIntf.h"

#ifndef __vmSIMULATOR__
  #include "RamHeap.h"
#endif

#include "Strings/TextStrs.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"

#include <stdio.h>
#include <string.h>
#include <DevTypes.h>
#include <GUI_Type.h>
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

//=============================================================================
// Layout constants
//-----------------------------------------------------------------------------
#define CAPTION_HEIGHT       24
#define CAPTION_WIDTH        20
#define EDGE_WIDTH           4

#define HEIGHT_LOGITEM       39

// Form rectangle (15,4,290,232) from spec
#define FORM_X1              15
#define FORM_Y1              4
#define FORM_X2              (FORM_X1 + 290 - 1)
#define FORM_Y2              (FORM_Y1 + 232 - 1)
#define FORM_W               290
#define FORM_H               232

// Caption area (vertical on left side)
#define CAPTION_X1           0    // relative to form
#define CAPTION_Y1           0
#define CAPTION_X2           (CAPTION_WIDTH - 1)
#define CAPTION_Y2           (FORM_H - 1)

// List area (remaining space)
#define LIST_X1              (CAPTION_WIDTH + EDGE_WIDTH)
#define LIST_Y1              EDGE_WIDTH
#define LIST_X2              (FORM_W - EDGE_WIDTH - 1)
#define LIST_Y2              (FORM_H - EDGE_WIDTH - 1)
#define LIST_W               (LIST_X2 - LIST_X1 + 1)
#define LIST_H               (LIST_Y2 - LIST_Y1 + 1)

#define NUM_VISIBLE_ITEMS    ((LIST_H) / HEIGHT_LOGITEM)

// Scrollbar
#define SCROLLBAR_WIDTH      8
#define SCROLLBAR_X1         (CAPTION_WIDTH + 2)
#define SCROLLBAR_Y1         (LIST_Y1)
#define SCROLLBAR_X2         (SCROLLBAR_X1 + SCROLLBAR_WIDTH - 1)
#define SCROLLBAR_Y2         (LIST_Y2)

//-----------------------------------------------------------------------------
// Colors
//-----------------------------------------------------------------------------
#define crFormBkg            (0x00102030)
#define crFormFrame          (0x002f5ca6)
#define crFormInner          (0x00183050)

#define crListBkg            (0x00181818)
#define crListText           GUI_CYAN
#define crListTextSel        (0x00003000)

// Selection colors (gradient)
#define crListSel0           (0x00D0DFFD)
#define crListSel1           (0x00BCCFFE)
#define crListSel2           (0x0069CEFF)
#define crListSel3           (0x0099D4FF)
#define crListSelEdg         (0x0000B7FF)

// Caption colors
#define crCaption1           GUI_LIGHTBLUE
#define crCaption2           GUI_LIGHTYELLOW
#define crCaption3           GUI_LIGHTRED
#define crCaptionBkg         (0x00102030)

// Scrollbar colors
#define crScrollbarBkg       (0x00203040)
#define crScrollbarThumb     (0x004080c0)

//-----------------------------------------------------------------------------
// Fonts
//-----------------------------------------------------------------------------
#define ftCaption            GUI_FONT_16LTH_CHN
#define ftListItem           GUI_FONT_16LTH_CHN

//=============================================================================
// Data structures
//-----------------------------------------------------------------------------
typedef struct tagLogListFormState
{
  TEventLogType  logType;        // Current log type (mltEvent/mltAlarm/mltFault)
  uint8_t        ucCount;        // Total log count
  uint8_t        ucTopItem;      // Top visible item index
  uint8_t        ucCurItem;      // Current selected item index
  uint32_t       uNextTick;      // Next update tick

  TEventWithProperty eventList[NUM_VISIBLE_ITEMS];  // Visible log cache
} TLogListFormState;

//=============================================================================
// Local data
//-----------------------------------------------------------------------------
static TLogListFormState *FpFormState = nullptr;

//=============================================================================
// Helper methods
//-----------------------------------------------------------------------------
// Get event list from EVTMGR
static void getEventList()
{
  memset( FpFormState->eventList, 0, sizeof(FpFormState->eventList) );

  for( uint32_t uIndex = 0; uIndex < NUM_VISIBLE_ITEMS; uIndex++ )
    {
    TEventLogItem *pEvent = &(FpFormState->eventList[uIndex].EvtLog);
    if( 0 != FIX_ReadEvtLogItem( FpFormState->logType,
                                 uIndex + FpFormState->ucTopItem,
                                 pEvent,
                                 ERD_BACKWARD ) )
      pEvent->Summary.State.RegNum = 0;
    }
}

//-----------------------------------------------------------------------------
// Get event info by index
static TEventWithProperty* getEventInfo( uint32_t uIndex )
{
  if( uIndex <  FpFormState->ucTopItem ||
      uIndex >= (uint32_t)(FpFormState->ucTopItem + NUM_VISIBLE_ITEMS) )
    return nullptr;

  TEventWithProperty *pEvent = FpFormState->eventList +
                               (uIndex - FpFormState->ucTopItem);

  if( 0 == pEvent->EvtLog.Summary.State.RegNum )
    return nullptr;

  return pEvent;
}

//-----------------------------------------------------------------------------
// Change log type
static void changeEventType( TEventLogType newType )
{
  FpFormState->logType   = newType;
  FpFormState->ucCount   = EVTMGR_GetEventCount( FpFormState->logType );
  FpFormState->ucTopItem = 0;
  FpFormState->ucCurItem = 0;
}

//-----------------------------------------------------------------------------
// Draw caption label (vertical text on left side)
static void drawCaptionLabel()
{
  uint32_t   uCapLabelId;
  GUI_COLOR  crCapLabel;

  switch( FpFormState->logType )
    {
    case mltEvent:
      uCapLabelId = idEventCatalog1;
      crCapLabel  = crCaption1;
      break;
    case mltAlarm:
      uCapLabelId = idEventCatalog2;
      crCapLabel  = crCaption2;
      break;
    case mltFault:
      uCapLabelId = idEventCatalog3;
      crCapLabel  = crCaption3;
      break;
    default:
      uCapLabelId = 0;
      break;
    }

  const char* pcStr = GetMultiLangString( uCapLabelId );
  if( nullptr != pcStr )
    {
    // Draw vertical caption text
    GUI_RECT rtCaption = {
      FORM_X1 + CAPTION_X1,
      FORM_Y1 + CAPTION_Y1,
      FORM_X1 + CAPTION_X2,
      FORM_Y1 + CAPTION_Y2
    };

    GUI_SetFont( ftCaption );
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_SetColor( crCapLabel );

    // Rotate text for vertical display
    GUI_SetTextAlign( GUI_TA_HCENTER | GUI_TA_VCENTER );

    // Calculate center position
    int cx = (rtCaption.x0 + rtCaption.x1) / 2;
    int cy = (rtCaption.y0 + rtCaption.y1) / 2;

    GUI_DispStringAt( pcStr, cx, cy );
    }
}

//-----------------------------------------------------------------------------
// Draw caption index (e.g., "1:5" showing current/total)
static void drawCaptionIndex()
{
  if( 0 < FpFormState->ucCount )
    {
    char szBuf[16];
    snprintf( szBuf, sizeof(szBuf), "%u:%u",
              FpFormState->ucCurItem + 1,
              FpFormState->ucCount );

    // Draw at bottom of caption area
    GUI_RECT rtIndex = {
      FORM_X1 + CAPTION_X1,
      FORM_Y1 + FORM_H - 20,
      FORM_X1 + CAPTION_X2,
      FORM_Y1 + FORM_H - 4
    };

    GUI_SetFont( GUI_FONT_13_ASCII );
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_SetColor( GUI_WHITE );
    GUI_DispStringInRect( szBuf, &rtIndex, GUI_TA_HCENTER | GUI_TA_VCENTER );
    }
}

//-----------------------------------------------------------------------------
// Draw scrollbar
static void drawScrollbar()
{
  if( FpFormState->ucCount <= NUM_VISIBLE_ITEMS )
    return;  // No scrollbar needed

  int x1 = FORM_X1 + SCROLLBAR_X1;
  int y1 = FORM_Y1 + SCROLLBAR_Y1;
  int x2 = FORM_X1 + SCROLLBAR_X2;
  int y2 = FORM_Y1 + SCROLLBAR_Y2;

  // Draw scrollbar background
  GUI_SetColor( crScrollbarBkg );
  GUI_FillRect( x1, y1, x2, y2 );

  // Calculate thumb position and size
  int trackH = y2 - y1 + 1;
  int thumbH = (NUM_VISIBLE_ITEMS * trackH) / FpFormState->ucCount;
  if( thumbH < 10 ) thumbH = 10;

  int thumbY1 = y1 + (FpFormState->ucTopItem * trackH) / FpFormState->ucCount;
  int thumbY2 = thumbY1 + thumbH - 1;
  if( thumbY2 > y2 ) thumbY2 = y2;

  // Draw thumb
  GUI_SetColor( crScrollbarThumb );
  GUI_FillRect( x1, thumbY1, x2, thumbY2 );
}

//-----------------------------------------------------------------------------
// Draw one log item
static void drawItemInfo( uint32_t uItemIndex )
{
  if( uItemIndex < FpFormState->ucTopItem ||
      uItemIndex >= (uint32_t)(FpFormState->ucTopItem + NUM_VISIBLE_ITEMS) )
    return;

  int itemOffset = uItemIndex - FpFormState->ucTopItem;

  short Item_x1 = FORM_X1 + LIST_X1 + EDGE_WIDTH;
  short Item_x2 = FORM_X1 + LIST_X2 - EDGE_WIDTH;
  short Item_y1 = FORM_Y1 + LIST_Y1 + itemOffset * HEIGHT_LOGITEM;
  short Item_y2 = Item_y1 + HEIGHT_LOGITEM - 1;

  if( Item_y2 > FORM_Y1 + LIST_Y2 - EDGE_WIDTH )
    Item_y2 = FORM_Y1 + LIST_Y2 - EDGE_WIDTH;

  // Draw background
  if( FpFormState->ucCurItem != uItemIndex )
    {
    // Normal item
    GUI_SetColor( crListBkg );
    GUI_FillRect( Item_x1, Item_y1, Item_x2, Item_y2 );
    }
  else
    {
    // Selected item with gradient
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + 2,
                       Item_x2 - 4, Item_y1 + HEIGHT_LOGITEM / 3,
                       crListSel0, crListSel1 );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + HEIGHT_LOGITEM / 3 + 1,
                       Item_x2 - 4, Item_y2 - 2,
                       crListSel2, crListSel3 );
    GUI_SetColor( crListSelEdg );
    GUI_DrawRoundedFrame( Item_x1, Item_y1, Item_x2, Item_y2, 2, 1 );
    }

  // Get event data
  TEventWithProperty* pEvent = getEventInfo( uItemIndex );
  if( nullptr == pEvent )
    return;

  // Get event description
  if( nullptr == pEvent->EvtProp )
    {
    if( 0 != EVTMGR_GetEventDesp( pEvent ) )
      return;

    if( nullptr == pEvent->RegInfo || 0 == pEvent->RegInfo->RegNum )
      return;
    }

  const TDevRegInfoItem* pInfo = DevIntf_GetRegInfo( pEvent->RegInfo->RegNum );
  if( nullptr == pInfo )
    return;

  // Get register name
  const char* pcRegName;
  if( nullptr != pInfo->pName )
    pcRegName = pInfo->pName;
  else
    pcRegName = GetMultiLangString( pInfo->NameStrId );

  if( nullptr == pcRegName )
    return;

  // Format log entry text
  char szStr[64];
  int iLen = snprintf( szStr, sizeof(szStr),
                       "%04u-%02u-%02u %02u:%02u:%02u:%03u\n",
                       pEvent->EvtLog.Summary.Time.Year,
                       pEvent->EvtLog.Summary.Time.Month,
                       pEvent->EvtLog.Summary.Time.Day,
                       pEvent->EvtLog.Summary.Time.Hours,
                       pEvent->EvtLog.Summary.Time.Minutes,
                       pEvent->EvtLog.Summary.Time.Seconds,
                       pEvent->EvtLog.Summary.Time.milSecs );
  snprintf( szStr + iLen, sizeof(szStr) - iLen, "%s %s", pcRegName, pEvent->EvtDesp );

  // Draw text
  if( FpFormState->ucCurItem != uItemIndex )
    GUI_SetColor( crListText );
  else
    GUI_SetColor( crListTextSel );

  GUI_SetFont( ftListItem );
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );

  GUI_RECT Rect = { short(Item_x1 + 16), Item_y1, Item_x2, Item_y2 };
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_LEFT | GUI_TA_VCENTER );
}

//-----------------------------------------------------------------------------
// Draw single item
static void drawItem( uint32_t uItemIndex )
{
  if( 0 == FpFormState->ucCount ||
      uItemIndex < FpFormState->ucTopItem ||
      (uint32_t)(FpFormState->ucTopItem + NUM_VISIBLE_ITEMS) <= uItemIndex )
    return;

  drawItemInfo( uItemIndex );
}

//-----------------------------------------------------------------------------
// Update entire list
static void updateList()
{
  getEventList();

#ifndef __vmSIMULATOR__
  osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  for( uint32_t uIdx = 0; uIdx < NUM_VISIBLE_ITEMS &&
                          uIdx < FpFormState->ucCount; uIdx++ )
    {
    drawItem( FpFormState->ucTopItem + uIdx );
    }

  drawCaptionIndex();
  drawScrollbar();

#ifndef __vmSIMULATOR__
  osMutexRelease(FGUIState.mutexGUI);
#endif
}

//-----------------------------------------------------------------------------
// Flush entire form
static void flushForm()
{
#ifndef __vmSIMULATOR__
  osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  // Draw background
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

  // Draw form frame
  GUI_SetColor( crFormFrame );
  GUI_DrawRect( FORM_X1, FORM_Y1, FORM_X2, FORM_Y2 );

  // Draw inner background
  GUI_SetColor( crFormInner );
  GUI_FillRect( FORM_X1 + 2, FORM_Y1 + 2, FORM_X2 - 2, FORM_Y2 - 2 );

  // Draw caption area
  GUI_SetColor( crCaptionBkg );
  GUI_FillRect( FORM_X1 + CAPTION_X1, FORM_Y1 + CAPTION_Y1,
                FORM_X1 + CAPTION_X2, FORM_Y1 + CAPTION_Y2 );

  drawCaptionLabel();

  // Draw list area
  GUI_SetColor( crListBkg );
  GUI_FillRect( FORM_X1 + LIST_X1, FORM_Y1 + LIST_Y1,
                FORM_X1 + LIST_X2, FORM_Y1 + LIST_Y2 );

  // Draw empty message if no logs
  if( 0 == FpFormState->ucCount )
    {
    GUI_RECT Rect = {
      FORM_X1 + LIST_X1, FORM_Y1 + LIST_Y1,
      FORM_X1 + LIST_X2, FORM_Y1 + LIST_Y2
    };
    const char* pcStr = GetMultiLangString( idEventEmpty );
    if( nullptr != pcStr )
      {
      GUI_SetFont( ftListItem );
      GUI_SetTextMode( GUI_TEXTMODE_TRANS );
      GUI_SetColor( crListText );
      GUI_DispStringInRect( pcStr, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER );
      }
    }

#ifndef __vmSIMULATOR__
  osMutexRelease(FGUIState.mutexGUI);
#endif
}

//=============================================================================
// Form lifecycle methods
//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{
#ifndef __vmSIMULATOR__
  FpFormState = (TLogListFormState*)RAM_Malloc( sizeof(TLogListFormState) );
#else
  FpFormState = new TLogListFormState;
#endif

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == FpFormState, GFC_OutOfMem );
#endif

  memset( FpFormState, 0, sizeof(TLogListFormState) );

  // Initialize with log type from argument (default mltFault)
  TEventLogType initType = (TEventLogType)(uintptr_t)argument;
  if( initType < mltEvent || initType > mltFault )
    initType = mltFault;

  changeEventType( initType );
}

//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{
  flushForm();
  updateList();
}

//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
#ifndef __vmSIMULATOR__
  RAM_Free(FpFormState);
#else
  delete FpFormState;
#endif

  FpFormState = nullptr;
}

//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
  // Update list every second
  if( uTick >= FpFormState->uNextTick )
    {
    FpFormState->uNextTick = uTick + 1000;

    // Refresh log count and update if changed
    uint8_t oldCount = FpFormState->ucCount;
    FpFormState->ucCount = EVTMGR_GetEventCount( FpFormState->logType );

    if( oldCount != FpFormState->ucCount )
      {
      updateList();
      }
    }
}

//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{
  uint32_t uCurrItem;

  switch( uwKey )
    {
    case KEY_UP:
      {
      if( FpFormState->ucCurItem > FpFormState->ucTopItem )
        {
        // Move selection up within visible range
        uCurrItem = FpFormState->ucCurItem;
        FpFormState->ucCurItem--;

        drawItem( uCurrItem );
        drawItem( FpFormState->ucCurItem );
        drawCaptionIndex();
        }
      else if( FpFormState->ucTopItem > 0 )
        {
        // Scroll up
        FpFormState->ucTopItem--;
        FpFormState->ucCurItem--;
        updateList();
        }
      else if( 1 < FpFormState->ucCount )
        {
        // Wrap to bottom
        FpFormState->ucCurItem = FpFormState->ucCount - 1;
        if( FpFormState->ucCount > NUM_VISIBLE_ITEMS )
          FpFormState->ucTopItem = FpFormState->ucCount - NUM_VISIBLE_ITEMS;
        else
          FpFormState->ucTopItem = 0;
        updateList();
        }
      break;
      }

    case KEY_DOWN:
      {
      if( FpFormState->ucCurItem < FpFormState->ucCount - 1 )
        {
        if( FpFormState->ucCurItem + 1 < FpFormState->ucTopItem + NUM_VISIBLE_ITEMS )
          {
          // Move selection down within visible range
          uCurrItem = FpFormState->ucCurItem;
          FpFormState->ucCurItem++;

          drawItem( uCurrItem );
          drawItem( FpFormState->ucCurItem );
          drawCaptionIndex();
          }
        else
          {
          // Scroll down
          FpFormState->ucCurItem++;
          FpFormState->ucTopItem++;
          updateList();
          }
        }
      else
        {
        // Wrap to top
        FpFormState->ucTopItem = 0;
        FpFormState->ucCurItem = 0;
        updateList();
        }
      break;
      }

    case KEY_RIGHT:
      {
      // Switch to next log type: Event->Alarm->Fault->WavelogForm
      TEventLogType nextType;
      if( FpFormState->logType == mltEvent )
        nextType = mltAlarm;
      else if( FpFormState->logType == mltAlarm )
        nextType = mltFault;
      else  // mltFault
        {
        // Switch to WLGListForm
        gfc::PopForm();
        gfc::PushForm( WID_WLGListForm, nullptr );
        return;
        }

      changeEventType( nextType );
      flushForm();
      updateList();
      break;
      }

    case KEY_LEFT:
      {
      // Switch to previous log type: Fault->Alarm->Event->MainForm
      TEventLogType prevType;
      if( FpFormState->logType == mltFault )
        prevType = mltAlarm;
      else if( FpFormState->logType == mltAlarm )
        prevType = mltEvent;
      else  // mltEvent
        {
        // Switch to MainForm
        gfc::PopForm();
        gfc::PushForm( WID_MainForm, nullptr );
        return;
        }

      changeEventType( prevType );
      flushForm();
      updateList();
      break;
      }
    }
}

//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{
  switch (uwKey)
    {
    case KEY_ESCAPE:
      gfc::PopForm();
      break;
    }
}

//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg )
{
  if( 0 == pMsg )
    return;

  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      {
      _OnTick( pMsg->Data.v );
      break;
      }

    case GM_KEYDOWN:
      {
      if( pMsg->Param )
        {
        _OnKeyDown( pMsg->Param );
        pMsg->MsgId = 0;
        }
      break;
      }

    case GM_KEYUP:
      {
      if (pMsg->Param)
        {
        _OnKeyUp(pMsg->Param);
        pMsg->MsgId = 0;
        }
      break;
      }
    }
}

//=============================================================================
// Form definition
//-----------------------------------------------------------------------------
const GWinForm FLogListForm =
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// Auto-register with GForm system
static const gfc::FormRegistrar kRegLogList(WID_LogListForm, &FLogListForm, "LogList");
//-----------------------------------------------------------------------------
