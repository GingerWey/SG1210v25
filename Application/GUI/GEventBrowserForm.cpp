//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPEventBrowserForm.cpp
 Version     : V1.10
 By          : Silver Grid Technology

 Description : Event browser form implementation.
               Displays device event/alert log.

 Date        : 2023.12.05
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GEventBrowserForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUICntr.h" // MCU: FGUIState, osWaitForever
#include "GUIConf.h"
//#include "FontCHS24LTH.h"
#include "FontCHS16LTH.h"

#include "DevFixed.h"
#include "DevEvtMgr.h"
#include "DevIntf.h"
//#include "DevRegInfo.h"
//#include "DevDebug.h"

#ifndef __vmSIMULATOR__
  #include "RamHeap.h"
#endif

#include "Strings/TextStrs.h"

#include <stdio.h>
#include <string.h>
#include <DevTypes.h>
#include <GUI_Type.h>
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"
//=============================================================================
// 
//-----------------------------------------------------------------------------
// Form
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION      24

#define WIDTH_EDGE          4

#define HEIGHT_EVENTITEM   39

#define NUM_VISIBLE_ITEMS  ((rtEventList_H - WIDTH_EDGE) / HEIGHT_EVENTITEM)
//-----------------------------------------------------------------------------
// Colors
#define crFormBkg         GUI_MAKE_COLOR(0x001F1F1F)
#define crFrameHigh       GUI_MAKE_COLOR(0x002F2F2F)
#define crFrameDrak       GUI_MAKE_COLOR(0x000F0F0F)

#define crSprit           GUI_GRAY_7C

// Caption
#define rtCaption_x1      0
#define rtCaption_y1      0
#define rtCaption_x2      (DESKTOP_WIDTH - 1)
#define rtCaption_y2      (rtCaption_x1 + HIGHT_CAPTION - 1)

#define rtCapLabel_x1     (rtCaption_x1 + CaptionIndex_W)
#define rtCapLabel_y1     (rtCaption_y1 + WIDTH_EDGE)
#define rtCapLabel_x2     (rtCaption_x2 - CaptionIndex_W)
#define rtCapLabel_y2     (rtCaption_y2 - WIDTH_EDGE)

#define CaptionIndex_W    100
#define rtCapIndex_x1     (rtCaption_x2 - CaptionIndex_W)
#define rtCapIndex_y1     (rtCapLabel_y1)
#define rtCapIndex_x2     (rtCaption_x2)
#define rtCapIndex_y2     (rtCapLabel_y2)
#define crCaptionIndex    GUI_WHITE

#define crCaptionLabel1   GUI_LIGHTBLUE
#define crCaptionLabel2   GUI_LIGHTYELLOW
#define crCaptionLabel3   GUI_LIGHTRED
#define ftCaptionLabel    GUI_FONT_16LTH_CHN

// Event List
#define rtListView_x1     (0 + WIDTH_EDGE)
#define rtListView_y1     (rtCaption_y2 + WIDTH_EDGE)
#define rtListView_x2     (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define rtListView_y2     (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define rtEventList_x1    (rtListView_x1 + WIDTH_EDGE)
#define rtEventList_y1    (rtListView_y1 + WIDTH_EDGE)
#define rtEventList_x2    (rtListView_x2 - WIDTH_EDGE)
#define rtEventList_y2    (rtListView_y2 - WIDTH_EDGE)

#define rtEventList_W     (rtEventList_x2 - rtEventList_x1 + 1)
#define rtEventList_H     (rtEventList_y2 - rtEventList_y1 + 1)

#define crEventBkg        GUI_MAKE_COLOR(0x00181818)

#define ftItemCaption     GUI_FONT_16LTH_CHN
#define crItemCaption     GUI_CYAN 
#define crItemCapSel      GUI_MAKE_COLOR(0x00003000)  //GUI_LIGHTGREEN 

#define crListSelBkg      GUI_GRAY_50
#define crListSelBkgHi    GUI_MAKE_COLOR(0xF0FFFF)
#define crListSelBkgEdg   GUI_MAKE_COLOR(0xB0E8E8)

#define crListBk          GUI_MAKE_COLOR(0x00E3E9FE)  // r254, g249, b243
#define crListSel0        GUI_MAKE_COLOR(0x00D0DFFD)  // r253, g239, b224
#define crListSel1        GUI_MAKE_COLOR(0x00BCCFFE)  // r253, g223, b188
#define crListSel2        GUI_MAKE_COLOR(0x0069CEFF)  // r255, g206, b105
#define crListSel3        GUI_MAKE_COLOR(0x0099D4FF)  // r255, g228, b153
#define crListSelEdg      GUI_MAKE_COLOR(0x0000B7FF)  // 

//=============================================================================
// Data
//-----------------------------------------------------------------------------
// ColumnStateData
typedef struct tagEventFormState
{
  uint8_t       ucType;
  uint8_t       ucCount;

  uint8_t       ucTopItem;
  uint8_t       ucCurItem;

  uint32_t      uNextTick;

  TEventWithProperty eventList[NUM_VISIBLE_ITEMS];
} TEventFormState;
//=============================================================================
// Data
//-----------------------------------------------------------------------------
// ColumnState
static TEventFormState *FpFormState = nullptr;
//=============================================================================
// Data
//-----------------------------------------------------------------------------
//=============================================================================
// 
//-----------------------------------------------------------------------------
static void getEventList()
{

  memset( FpFormState->eventList, 0, sizeof(FpFormState->eventList) );
  
  for( uint32_t uIndex = 0; uIndex < NUM_VISIBLE_ITEMS; uIndex++ )
    {
    TEventLogItem *pEvent = &(FpFormState->eventList[uIndex].EvtLog);
    if( 0 != FIX_ReadEvtLogItem( (TEventLogType)(FpFormState->ucType), 
                                 uIndex + FpFormState->ucTopItem,
                                 pEvent,
                                 ERD_BACKWARD ) )
      pEvent->Summary.State.RegNum = 0;
    }
}
//-----------------------------------------------------------------------------
static TEventWithProperty* getEventInfo( uint32_t uIndex )
{
  
  if( uIndex <  FpFormState->ucTopItem ||
      uIndex >= FpFormState->ucTopItem + NUM_VISIBLE_ITEMS )
    return nullptr;

  TEventWithProperty *pEvent = FpFormState->eventList + 
                               (uIndex - FpFormState->ucTopItem);
  
  if( 0 == pEvent->EvtLog.Summary.State.RegNum )
    return nullptr;
  
  return pEvent;
}
//-----------------------------------------------------------------------------
static void changeEventType( TEventLogType newType )
{
  
  FpFormState->ucType  = newType;
  FpFormState->ucCount = EVTMGR_GetEventCount( (TEventLogType)(FpFormState->ucType) );
  FpFormState->ucTopItem = 0;
  FpFormState->ucCurItem = 0;
}
//-----------------------------------------------------------------------------
static void drawCaptionLabel()
{

#if HIGHT_CAPTION > 0
  uint32_t   uCaplabelId;
  GUI_COLOR  crCaplabel;
  switch( (TEventLogType)(FpFormState->ucType) )
    {
    case mltEvent:
      uCaplabelId = idEventCatalog1;
      crCaplabel  = crCaptionLabel1;
      break;
    case mltAlarm:
      uCaplabelId = idEventCatalog2;
      crCaplabel  = crCaptionLabel1;
      break;
    case mltFault:
      uCaplabelId = idEventCatalog3;
      crCaplabel  = crCaptionLabel1;
      break;
   default:
      uCaplabelId = 0;
      break;
    }

  const char* pcStr = GetMultiLangString( uCaplabelId );
  if( nullptr != pcStr )
    {
    GUI_RECT rtCapLabel = { rtCapLabel_x1, rtCapLabel_y1, 
                            rtCapLabel_x2, rtCapLabel_y2 };

    GUI_SetFont    ( ftCaptionLabel ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_SetColor   ( crCaplabel );
    GUI_DispStringInRect( pcStr, &rtCapLabel, GUI_TA_CENTER | GUI_TA_VCENTER );
    }
#endif
}
//-----------------------------------------------------------------------------
static void drawCaptionIndex(bool bEarse)
{

#if HIGHT_CAPTION > 0
  if( true == bEarse )
    {
    GUI_SetColor( crFormBkg );
    GUI_FillRect( rtCapIndex_x1, rtCapIndex_y1, 
                  rtCapIndex_x2, rtCapIndex_y2 );
    }
  
  if( 0 < FpFormState->ucCount )
    {
    GUI_RECT rtCapIndex = { rtCapIndex_x1, rtCapIndex_y1, 
                            rtCapIndex_x2, rtCapIndex_y2 };
    
    char szBuf[10];
    sprintf( szBuf, "%u:%u", 
             FpFormState->ucCurItem + 1, 
             FpFormState->ucCount );
    
    GUI_SetFont    ( ftCaptionLabel ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
    GUI_SetColor( crCaptionIndex );
    GUI_DispStringInRect( szBuf, &rtCapIndex, GUI_TA_CENTER | GUI_TA_VCENTER );
    }
#endif
}
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
#ifndef __vmSIMULATOR__
    osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

  // Caption
  drawCaptionLabel();
//  drawCaptionIndex(false);

  // Caption split line
  GUI_SetColor( crFrameHigh );
  GUI_SetPenSize( 1 );
  GUI_SetPenShape( GUI_PS_SQUARE );
  GUI_SetLineStyle( GUI_LS_SOLID );
  GUI_DrawLine( rtCaption_x1, rtCaption_y2 + 1, 
                rtCaption_x2, rtCaption_y2 + 1 );

  // Event list frame
  GUI_DrawRect( rtListView_x1, rtListView_y1, rtListView_x2, rtListView_y2 );

  // event list view
  GUI_SetColor( crEventBkg );
  GUI_FillRect( rtEventList_x1, rtEventList_y1, 
                rtEventList_x2, rtEventList_y2 );  

  // null event list
  if( 0 == FpFormState->ucCount )
    {
    GUI_RECT Rect = { rtEventList_x1, rtEventList_y1, 
                      rtEventList_x2, rtEventList_y2 };
    const char* pcStr = GetMultiLangString( idEventEmpty );
    if( nullptr != pcStr )
      {
      GUI_SetFont    ( ftCaptionLabel ); 
      GUI_SetTextMode( GUI_TEXTMODE_TRANS );
      GUI_SetColor   ( crItemCaption );
      GUI_DispStringInRect( pcStr, &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
      }
    }

#ifndef __vmSIMULATOR__
    osMutexRelease(FGUIState.mutexGUI);
#endif
}
//-----------------------------------------------------------------------------
static void _DrawItemInfo(uint32_t uItemIndex)
{
  
  short Item_x1 = rtEventList_x1 + WIDTH_EDGE,
        Item_x2 = rtEventList_x2 - WIDTH_EDGE,
        Item_y1 = rtEventList_y1 + WIDTH_EDGE + 
                  (uItemIndex - FpFormState->ucTopItem) * HEIGHT_EVENTITEM,
        Item_y2 = Item_y1 + HEIGHT_EVENTITEM - 1;
  if( Item_y2 > rtEventList_y2 - WIDTH_EDGE )
    Item_y2 = rtEventList_y2 - WIDTH_EDGE;
      
  if( FpFormState->ucCurItem != uItemIndex )
    {
    GUI_SetColor( crEventBkg );
    GUI_FillRect( Item_x1, Item_y1, Item_x2, Item_y2 );
    }
  else
    {
    GUI_SetColor( crListBk );
    GUI_SetBkColor( crListBk );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + 2, 
                       Item_x2 - 4, Item_y1 + HEIGHT_EVENTITEM / 3, 
                       crListSel0,   crListSel1 );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + HEIGHT_EVENTITEM / 3 + 1, 
                       Item_x2 - 4, Item_y2 - 2,
                       crListSel2, crListSel3 );
    GUI_SetColor( crListSelEdg );
    GUI_DrawRoundedFrame( Item_x1, Item_y1, 
                          Item_x2, Item_y2, 2, 1 );
    }

  // Event
  TEventWithProperty* pEvent = getEventInfo( uItemIndex );
  if( nullptr == pEvent )
    return ;
  
  if( nullptr == pEvent->EvtProp )
    {
    if( 0 != EVTMGR_GetEventDesp( pEvent ) )
      return ;
    
    if( nullptr == pEvent->RegInfo || 0 == pEvent->RegInfo->RegNum )
      return;
    }

  const TDevRegInfoItem* pInfo = DevIntf_GetRegInfo( pEvent->RegInfo->RegNum );
  if( nullptr == pInfo )
    {
#ifdef USE_DEV_ASSERT
    DEV_FAULT( GFC_EmptyPtr );
#endif
    return ;
    }

  const char* pcRegName;
  if( nullptr != pInfo->pName )
    pcRegName = pInfo->pName;
  else
    pcRegName = GetMultiLangString( pInfo->NameStrId );
  
  if( nullptr == pcRegName )
    {
#ifdef USE_DEV_ASSERT
    DEV_FAULT( GFC_EmptyPtr );
#endif
    return ;
    }
    
  char szStr[64];
  int iLen = sprintf( szStr,
                      "%04u-%02u-%02u %02u:%02u:%02u:%03u\n",
                      pEvent->EvtLog.Summary.Time.Year,
                      pEvent->EvtLog.Summary.Time.Month,
                      pEvent->EvtLog.Summary.Time.Day,
                      pEvent->EvtLog.Summary.Time.Hours,
                      pEvent->EvtLog.Summary.Time.Minutes,
                      pEvent->EvtLog.Summary.Time.Seconds,
                      pEvent->EvtLog.Summary.Time.milSecs );
  sprintf( szStr + iLen, "%s %s", pcRegName, pEvent->EvtDesp );

  // Draw
  if( FpFormState->ucCurItem != uItemIndex )
    GUI_SetColor( crItemCaption );
  else
    GUI_SetColor( crItemCapSel );
    
  GUI_SetFont    ( ftItemCaption );
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );

  GUI_RECT Rect = { short(Item_x1 + 16), Item_y1, Item_x2, Item_y2 };
  GUI_DispStringInRect( szStr, &Rect, GUI_TA_LEFT | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _DrawItem(uint32_t uItemIndex)
{
  
  if( 0 == FpFormState->ucCount           ||
      uItemIndex < FpFormState->ucTopItem ||
      FpFormState->ucTopItem + NUM_VISIBLE_ITEMS <= uItemIndex )
    return ;  

  _DrawItemInfo( uItemIndex );
}
//-----------------------------------------------------------------------------
static void _UpdateList()
{

  // UpdateEvent
  getEventList();
  
#ifndef __vmSIMULATOR__
  osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  for( uint32_t uIdx = 0; uIdx < FpFormState->ucCount; uIdx++ )
    {
    _DrawItem( FpFormState->ucTopItem + uIdx );
    }

  drawCaptionIndex( true );

#ifndef __vmSIMULATOR__
  osMutexRelease(FGUIState.mutexGUI);
#endif
}
//=============================================================================
// Global methods
//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{

#ifndef __vmSIMULATOR__
    // 
  FpFormState = (TEventFormState*)RAM_Malloc( sizeof(TEventFormState) );
#else
    FpFormState = new TEventFormState;
#endif

#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == FpFormState, GFC_OutOfMem );
#endif

  memset( FpFormState, 0, sizeof(TEventFormState) );
  
  // EventType
  changeEventType( mltEvent );
  
  FpFormState->ucCurItem = (uint32_t)argument;
  if( FpFormState->ucCount <= FpFormState->ucCurItem )
    FpFormState->ucCurItem = 0;
}
//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{

  FpFormState->ucCurItem = (uint32_t)argument;
  if( FpFormState->ucCount <= FpFormState->ucCurItem )
    FpFormState->ucCurItem = 0;
  
  _FlushForm();

  _UpdateList();
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
}
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{

  uint32_t uCurrItem;
  
  switch( uwKey )
    {
    //case KEY_ESCAPE:
    //  GUIFormSwitch( WID_MainForm, 0 );
    //  break;

    case KEY_UP:
      {
      if( FpFormState->ucCurItem > FpFormState->ucTopItem )
        {
        uCurrItem = FpFormState->ucCurItem;
        FpFormState->ucCurItem--;
        
        _DrawItem( uCurrItem );
        _DrawItem( FpFormState->ucCurItem );

        drawCaptionIndex( true );
        }
      else if( FpFormState->ucTopItem > 0 )
        {
        FpFormState->ucTopItem--;
        FpFormState->ucCurItem--;

        _UpdateList();
        }
      else if( 1 < FpFormState->ucCount )
        {
        FpFormState->ucCurItem = FpFormState->ucCount - 1;
        if( FpFormState->ucCount > NUM_VISIBLE_ITEMS )
          FpFormState->ucTopItem = FpFormState->ucCount - NUM_VISIBLE_ITEMS;
        else
          FpFormState->ucTopItem = 0;

        _UpdateList();
        }
      
      break;
      }

    case KEY_DOWN:
      {
      if( FpFormState->ucCurItem < FpFormState->ucCount - 1 )
        {
        if( FpFormState->ucCurItem + 1 < FpFormState->ucTopItem + NUM_VISIBLE_ITEMS)
          {
          uCurrItem = FpFormState->ucCurItem;
          FpFormState->ucCurItem++;
          
          _DrawItem( uCurrItem );
          _DrawItem( FpFormState->ucCurItem );

          drawCaptionIndex( true );
          }
        else
          {
          FpFormState->ucCurItem++;
          FpFormState->ucTopItem++;
          
          _UpdateList();
          }
        }
      else
        {
        FpFormState->ucTopItem = 0;
        FpFormState->ucCurItem = 0;
        _UpdateList();
        }

      break;
      }

    case KEY_RIGHT:
      {
      TEventLogType eltNew;
      if( FpFormState->ucType < mltFault )
        eltNew = (TEventLogType)(FpFormState->ucType + 1);
      else
        eltNew = mltEvent;
      
      changeEventType( eltNew );
      
      _FlushForm();

      _UpdateList();

      break;
      }

    case KEY_LEFT:
      {
      TEventLogType eltNew;
      if( FpFormState->ucType > mltEvent )
        eltNew = (TEventLogType)(FpFormState->ucType - 1);
      else
        eltNew = mltFault;
      
      changeEventType( eltNew );
      
      _FlushForm();

      _UpdateList();

      break;
      }

    case KEY_ENTER:
      {

      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{

    switch (uwKey) {
    case KEY_ESCAPE:
        gfc::PopForm();
        break;
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
// Form
//-----------------------------------------------------------------------------
const GWinForm FEventBrowserForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// Auto-register with new GForm system
static const gfc::FormRegistrar kRegEvents(WID_EventListForm, &FEventBrowserForm, "EventBrowser");
//-----------------------------------------------------------------------------
