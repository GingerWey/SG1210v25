//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPMenuForm.c
 Version     : V1.11
 By          : Silver Grid Technology

 Description : Main menu form implementation.
               Lists menu items, navigates to sub-forms on selection.

 Date        : 2023.12.05 (V1.10 — original implementation)
              2026.06.25 (V1.11 — added GM_TOUCH touch screen handler)
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GPMenuForm.h"

#include "GUI.h"
#include "GUIBitMap.h"
#include "GForm.h"
#include "GUICntr.h" // MCU: FGUIState, osWaitForever
#include "GUIConf.h"
#include "GUIMessage.h"

#include "FontSGRes.h"
#include "PictureRes.h"
#include "Strings/TextStrs.h"

#include <string.h>
#include "GWinTypes.h"
#include "GFormRegistrar.h"
//=============================================================================
// 
//-----------------------------------------------------------------------------
// Menu ucCommand
#define MC_NONE         0x00
#define MC_OPENFORM     0x01
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Form
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     48

#define WIDTH_EDGE        4

#define HEIGHT_MENUITEM   40

#define VISIBLE_ITEMS     ((MenuList_H + - WIDTH_EDGE * 2 + (HEIGHT_MENUITEM - 1)) / HEIGHT_MENUITEM)
//-----------------------------------------------------------------------------
#define Caption_x1      0
#define Caption_y1      0
#define Caption_x2      (DESKTOP_WIDTH - 1)
#define Caption_y2      (Caption_x1 + HIGHT_CAPTION - 1)

#define CapLabel_x1     (Caption_x1 +  WIDTH_EDGE)
#define CapLabel_y1     (Caption_y1 +  WIDTH_EDGE)
#define CapLabel_x2     (Caption_x2 -  WIDTH_EDGE)
#define CapLabel_y2     (Caption_y2 -  WIDTH_EDGE)

// Menu List
#define ListView_x1     (0 + WIDTH_EDGE)
#define ListView_y1     (Caption_y2 + WIDTH_EDGE)
#define ListView_x2     (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define ListView_y2     (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define MenuList_x1     (ListView_x1 +  WIDTH_EDGE)
#define MenuList_y1     (ListView_y1 +  WIDTH_EDGE)
#define MenuList_x2     (ListView_x2 -  WIDTH_EDGE)
#define MenuList_y2     (ListView_y2 -  WIDTH_EDGE)

#define MenuList_W      (MenuList_x2 - MenuList_x1 + 1)
#define MenuList_H      (MenuList_y2 - MenuList_y1 + 1)

// Colors
#define crFormBkg       GUI_MAKE_COLOR(0x001F1F1F)  
#define crFrameHigh     GUI_MAKE_COLOR(0x002F2F2F)  
#define crFrameDrak     GUI_MAKE_COLOR(0x000F0F0F)

#define crCaptionFont   GUI_GRAY_9A  
#define ftCaption       GUI_FONT_24LTH_CHN

#define crMenuBkg       GUI_MAKE_COLOR(0x00181818)

#define ftItemCaption   GUI_FONT_24LTH_CHN
#define crItemCaption   GUI_CYAN 
#define crItemCapSel    GUI_MAKE_COLOR(0x00003000)  //GUI_LIGHTGREEN 

#define crSprit         GUI_GRAY_7C

#define crListSelBkg    GUI_GRAY_50
#define crListSelBkgHi  GUI_MAKE_COLOR(0xF0FFFF)
#define crListSelBkgEdg GUI_MAKE_COLOR(0xB0E8E8)

#define crListBk        GUI_MAKE_COLOR(0x00E3E9FE)  // r254, g249, b243
#define crListSel0      GUI_MAKE_COLOR(0x00D0DFFD)  // r253, g239, b224
#define crListSel1      GUI_MAKE_COLOR(0x00BCCFFE)  // r253, g223, b188
#define crListSel2      GUI_MAKE_COLOR(0x0069CEFF)  // r255, g206, b105
#define crListSel3      GUI_MAKE_COLOR(0x0099D4FF)  // r255, g228, b153
#define crListSelEdg    GUI_MAKE_COLOR(0x0000B7FF)  // 

//=============================================================================
// Data
//-----------------------------------------------------------------------------
// ColumnStateData
typedef struct tagMenuFormState
{

  uint32_t     uTopItem;
  uint32_t     uCurItem;
  
  uint32_t     uCount;
} TMenuFormState;
//-----------------------------------------------------------------------------
typedef struct tagGMenuItem
{
  uint32_t     uCapIdent;

  uint8_t      ucOption;

  uint8_t      ucCommand;
  uint16_t     Param1;
  uint32_t     Param2;
} GMenuItem;
//=============================================================================
// Data
//-----------------------------------------------------------------------------
// ColumnState
static TMenuFormState m_FormState = {0};
//-----------------------------------------------------------------------------
static const GMenuItem MenuItems[] = 
{
   { idMenuName1,   // Event
     0,
     MC_OPENFORM,
     WID_EventListForm,
     0
   }
  ,{ idMenuName2,   // Parameter
     0,
     MC_OPENFORM,
     WID_CTRLConfigForm,
     0
   }
  ,{ idMenuName3,   // Parameter
     0,
     MC_OPENFORM,
     WID_UARTConfigForm,
     0
   }
  ,{ idMenuName4,   // 
     0,
     MC_OPENFORM,
     WID_DevInfoForm,
     0
   }
};
#define NUM_MenuItems (sizeof(MenuItems) / sizeof(MenuItems[0]))
//=============================================================================
// Data
//-----------------------------------------------------------------------------
//=============================================================================
// 
//-----------------------------------------------------------------------------
static void _FlushForm()
{

#ifndef __vmSIMULATOR__
    osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif

  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

#if HIGHT_CAPTION > 0
  const char *pCapStr = GetMultiLangString( idMenuCaption );

  if( nullptr != pCapStr )
    {
    GUI_SetColor( crFrameHigh );
    GUI_RECT Rect1 = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
    GUI_DrawRectEx( &Rect1 );
    
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );

    GUI_DispStringInRect( pCapStr, &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
    }
#endif

  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( ListView_x1, ListView_y1, ListView_x2, ListView_y2 );

  GUI_SetColor( crMenuBkg );
  GUI_FillRect( MenuList_x1, MenuList_y1, MenuList_x2, MenuList_y2 );  

#ifndef __vmSIMULATOR__
  osMutexRelease(FGUIState.mutexGUI);
#endif
}
//-----------------------------------------------------------------------------
static void _DrawItemCaption(uint32_t uItemIndex)
{
  
  short Item_x1 = MenuList_x1 + WIDTH_EDGE,
        Item_x2 = MenuList_x2 - WIDTH_EDGE,
        Item_y1 = MenuList_y1 + WIDTH_EDGE + 
                  (uItemIndex - m_FormState.uTopItem) * HEIGHT_MENUITEM,
        Item_y2 = Item_y1 + HEIGHT_MENUITEM - 1;
  if( Item_y2 > MenuList_y2 - WIDTH_EDGE )
    Item_y2 = MenuList_y2 - WIDTH_EDGE;
      
  if( m_FormState.uCurItem != uItemIndex )
    {
    GUI_SetColor( crMenuBkg );
    GUI_FillRect( Item_x1, Item_y1, Item_x2, Item_y2 );
    }
  else
    {
    GUI_SetColor  ( crListBk );
    GUI_SetBkColor( crListBk );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + 2, 
                       Item_x2 - 4, Item_y1 + HEIGHT_MENUITEM / 3, 
                       crListSel0,   crListSel1 );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + HEIGHT_MENUITEM / 3 + 1, 
                       Item_x2 - 4, Item_y2 - 2,
                       crListSel2, crListSel3 );
    GUI_SetColor( crListSelEdg );
    GUI_DrawRoundedFrame( Item_x1, Item_y1, 
                          Item_x2, Item_y2, 2, 1 );
    }

  const char *pCapStr = GetMultiLangString( MenuItems[uItemIndex].uCapIdent );
  if( nullptr != pCapStr )
    {
    if( m_FormState.uCurItem != uItemIndex )
      GUI_SetColor( crItemCaption );
    else
      GUI_SetColor( crItemCapSel );
      
    GUI_SetFont ( ftItemCaption );
      
    GUI_RECT Rect = { Item_x1, Item_y1, Item_x2, Item_y2 };
    GUI_DispStringInRect( pCapStr, &Rect, 
                          GUI_TA_CENTER | GUI_TA_VCENTER);
    }
}
//-----------------------------------------------------------------------------
static void _DrawItem(uint32_t uItemIndex)
{
  
  if( uItemIndex < m_FormState.uTopItem || 
      m_FormState.uTopItem + VISIBLE_ITEMS < uItemIndex )
    return ;  

  _DrawItemCaption( uItemIndex );
}
//-----------------------------------------------------------------------------
static void _UpdateList()
{

#ifndef __vmSIMULATOR__
    osMutexWait(FGUIState.mutexGUI, osWaitForever);
#endif
  
  for( uint32_t uIdx = 0; uIdx < NUM_MenuItems; uIdx++ )
    {
    _DrawItem( m_FormState.uTopItem + uIdx );
    }

#ifndef __vmSIMULATOR__
    osMutexRelease(FGUIState.mutexGUI);
#endif
}
//-----------------------------------------------------------------------------
static void _ExecuteMenuCommand()
{

  const GMenuItem *MenuItem = &MenuItems[m_FormState.uCurItem];

  switch( MenuItem->ucCommand )
    {
    case MC_OPENFORM:
      {
      if( WID_FORMBEGIN <= MenuItem->Param1 )
        gform::PushForm( MenuItem->Param1, (const void*)MenuItem->Param2 );
      break;
      }
    }
}
//=============================================================================
// Global methods
//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{

  memset( &m_FormState, 0, sizeof(m_FormState) );
  m_FormState.uCount = NUM_MenuItems;

  m_FormState.uCurItem = (uint32_t)argument;
  if( NUM_MenuItems <= m_FormState.uCurItem )
    m_FormState.uCurItem = 0;
}
//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{

  m_FormState.uCurItem = (uint32_t)argument;
  if( NUM_MenuItems <= m_FormState.uCurItem )
    m_FormState.uCurItem = 0;

  _FlushForm();

  _UpdateList();
}
//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
}
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{

  uint8_t ucCur;

  switch( uwKey )
    {
    //case KEY_LEFT:
    //case KEY_ESCAPE:
    //  {
    //  gform::PushForm( WID_MainForm, 0 );
    //  break;
    //  }

    case KEY_UP:
      {
      if( m_FormState.uCurItem > m_FormState.uTopItem )
        {
        ucCur = m_FormState.uCurItem;
        m_FormState.uCurItem--;
        
        _DrawItem( ucCur );
        _DrawItem( m_FormState.uCurItem );
        }
      else if( m_FormState.uTopItem > 0 )
        {
        m_FormState.uTopItem--;
        m_FormState.uCurItem--;
        _UpdateList();
        }
      else if( 1 < m_FormState.uCount )
        {
        m_FormState.uCurItem = m_FormState.uCount - 1;
        if( m_FormState.uCurItem + 1 > VISIBLE_ITEMS )
          m_FormState.uTopItem = m_FormState.uCurItem + 1 - VISIBLE_ITEMS;
        else
          m_FormState.uTopItem = 0;

        _UpdateList();
        }

      break;
      }

    case KEY_DOWN:
      {
      if( m_FormState.uCurItem < m_FormState.uCount - 1 )
        {
        if( m_FormState.uCurItem + 1 < m_FormState.uTopItem + VISIBLE_ITEMS)
          {
          ucCur = m_FormState.uCurItem;
          m_FormState.uCurItem++;
          
          _DrawItem( ucCur );
          _DrawItem( m_FormState.uCurItem );
          }
        else
          {
          m_FormState.uCurItem++;
          m_FormState.uTopItem++;
          
          _UpdateList();
          }
        }
      else
        {
        m_FormState.uTopItem = 0;
        m_FormState.uCurItem = 0;
        _UpdateList();
        }

      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{

    switch (uwKey) {
    case KEY_ESCAPE: {
        gform::PopForm();
        break;
      }

    case KEY_ENTER: {
        _ExecuteMenuCommand();

        break;
    }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE * pMsg)
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
    case GM_KEYUP: {
        if (pMsg->Param) {
            _OnKeyUp(pMsg->Param);

            pMsg->MsgId = 0;
        }

        break;
    }

#if GUI_SUPPORT_TOUCH
    case GM_TOUCH: {
        uint16_t x = static_cast<uint16_t>((pMsg->Data.v >> 16) & 0xFFFF);
        uint16_t y = static_cast<uint16_t>(pMsg->Data.v & 0xFFFF);
        (void)x;  // Menu list uses full width

        // Map y coordinate to visible item index
        int visIdx = (static_cast<int>(y) - MenuList_y1) / HEIGHT_MENUITEM;
        if (visIdx < 0) visIdx = 0;

        int itemIdx = static_cast<int>(m_FormState.uTopItem) + visIdx;
        if (itemIdx >= static_cast<int>(m_FormState.uCount))
            itemIdx = static_cast<int>(m_FormState.uCount) - 1;
        if (itemIdx < 0) break;

        if (pMsg->Param == TOUCH_DOWN || pMsg->Param == TOUCH_MOVE) {
            // Move cursor to touched item
            if (static_cast<uint32_t>(itemIdx) != m_FormState.uCurItem) {
                uint32_t oldCur = m_FormState.uCurItem;
                m_FormState.uCurItem = static_cast<uint32_t>(itemIdx);
                _DrawItem(oldCur);
                _DrawItem(m_FormState.uCurItem);
            }
        } else if (pMsg->Param == TOUCH_UP) {
            // Select and execute
            m_FormState.uCurItem = static_cast<uint32_t>(itemIdx);
            _ExecuteMenuCommand();
        }
        pMsg->MsgId = 0;
        break;
    }
#endif
    }
}
//=============================================================================
// Form
//-----------------------------------------------------------------------------
const GWinForm FMainMenuForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// Auto-register with new GForm system
static const gform::FormRegistrar kRegMenu(WID_MenuForm, &FMainMenuForm, "Menu");
//-----------------------------------------------------------------------------
