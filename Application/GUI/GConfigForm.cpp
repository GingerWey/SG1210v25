//-----------------------------------------------------------------------------
/*
 File        : GPConfig.c
 Version     : V1.10
 By          : 银网科技
 Description: 参数配置窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GConfigForm.h"

#include "GUI.h"
#include "GUIConf.h"
#include "GUICntr.h"
#include "FontCHS12x12HT.h"
#include "FontCHS24x24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"

#include "DevRegs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define  GetRegisterValue(x)      DevConfig.Configs[(x) - REG_DEVCONFIG]
#define  SetRegisterValue(x, v)   DevCfgForEdit.Configs[(x) - REG_DEVCONFIG] = (v)
//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     40

#define WIDTH_EDGE        4
#define HEIGHT_ITEM_MAGIN (WIDTH_EDGE / 2)

#define HEIGHT_MENUITEM   28

#define VISIBLE_ITEMS     ((ConfigList_H + - WIDTH_EDGE * 2 + (HEIGHT_MENUITEM - 1)) / HEIGHT_MENUITEM)
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

#define ConfigList_x1   (ListView_x1 +  WIDTH_EDGE)
#define ConfigList_y1   (ListView_y1 +  WIDTH_EDGE)
#define ConfigList_x2   (ListView_x2 -  WIDTH_EDGE)
#define ConfigList_y2   (ListView_y2 -  WIDTH_EDGE)

#define ConfigList_W    (ConfigList_x2 - ConfigList_x1 + 1)
#define ConfigList_H    (ConfigList_y2 - ConfigList_y1 + 1)

#define Width_ItemCaption   (ConfigList_W - Width_ItemEditor - Width_ItemTail)
#define Width_ItemEditor    90    //
#define Width_ItemTail      80    // 

#define ItemCaption_x1  (ConfigList_x1 + WIDTH_EDGE)
#define ItemCaption_x2  (ItemCaption_x1 + Width_ItemCaption - 2 * WIDTH_EDGE)

#define ItemEditor_x1   (ConfigList_x1 + Width_ItemCaption + WIDTH_EDGE)
#define ItemEditor_x2   (ItemEditor_x1 + Width_ItemEditor - 2* WIDTH_EDGE)

#define ItemTail_x1     (ConfigList_x2 - Width_ItemTail + WIDTH_EDGE)
#define ItemTail_x2     (ConfigList_x2 - WIDTH_EDGE)

// Colors
#define crFormBkg       0x001F1F1F  
#define crFrameHigh     0x002F2F2F  
#define crFrameDrak     0x000F0F0F

#define crCaptionFont   GUI_GRAY_9A  
#define ftCaption      &GUI_CHSLTHFont24

#define crListBkg       0x00181818 

#define ftItemCaption  &GUI_CHSLTHFont24
#define crItemCaption   GUI_LIGHTCYAN   // GUI_DARKCYAN 
#define crItemCapSel    0x003000        // GUI_LIGHTYELLOW

#define ftItemValue    &GUI_CHSLTHFont24
#define crItemValueSel1 GUI_LIGHTCYAN
#define crItemValueSel2 0xB83CE0        //GUI_BLUE

#define crSprit         GUI_GRAY_7C

#define crListSelBkg    GUI_GRAY_50
#define crListSelBkgHi  0xF0FFFF
#define crListSelBkgEdg 0xB0E8E8

#define crListBk        0xE3E9FE  // r254, g249, b243
#define crListSel0      0xD0DFFD  // r253, g239, b224
#define crListSel1      0xBCCFFE  // r253, g223, b188
#define crListSel2      0x69CEFF  // r255, g206, b105
#define crListSel3      0x99D4FF  // r255, g228, b153
#define crListSelEdg    0x00B7FF  // 

#define crEditorBkg     0x00303030 
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 列表状态数据
typedef struct __GListState
{

  uint8_t     FTopItem;
  uint8_t     FCurItem;
  uint16_t    FCount;
  
  uint16_t    FTick;
  uint32_t    FNextTick;
  
  uint8_t     FEditing;
  uint8_t     FDigitNo;
  
  uint16_t    FEditValue;
} GListState;
//-----------------------------------------------------------------------------
typedef struct __GConfigItem
{
  const char*  Caption;
  const char*  Tail;
  
  uint16_t     Option;
  
  uint16_t     Register;
  uint16_t     Step, Min, Max;
} GConfigItem;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 列表状态
static GListState m_ListState = {0};
//-----------------------------------------------------------------------------
static const GConfigItem ConfigItems[] = 
{
   { "CT变比:",
     "/ 5",   
     0,
     REG_CT_RATE,
     5,
     25,
     10000
   }
  ,{ "启动前延时:",
     "秒",   
     0,
     REG_TEST_IgniteWait,
     1,
     1,
     30
   }
  ,{ "试验循环次数:",
     "次",   
     0,
     REG_TEST_IgniteCicles,
     1,
     1,
     99
   }
  ,{ "燃弧持续时间:",
     "周期",   
     0,
     REG_TEST_Ignite,
     1,
     1,
     99
   }
  ,{ "熄弧后间隔时间:",
     "周期",   
     0,
     REG_TEST_IgnitePause,
     1,
     1,
     99
   }
  ,{ "断路器合闸等待遥信:",
     "ms",   
     0,
     REG_TEST_BreakerONWait,
     10,
     0,
     10000
   }
  ,{ "断路器分闸等待遥信:",
     "ms",   
     0,
     REG_TEST_BreakerOFFWait,
     10,
     0,
     10000
   }
  ,{ "点火后等待电流产生:",
     "ms",   
     0,
     REG_TEST_IgniteWaitCurrent,
     100,
     0,
     10000
   }
  ,{ "吹弧启动后等待电流消失:",
     "ms",   
     0,
     REG_Test_MagnetBlastWait,
     100,
     100,
     10000
   }
};
#define NUM_ConfigItems (sizeof(ConfigItems) / sizeof(ConfigItems[0]))
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
  osMutexWait( FGUIState.FLCDMutex, osWaitForever );
  
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT Rect1 = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
  GUI_DrawRectEx( &Rect1 );

  GUI_SetColor( crCaptionFont );
  GUI_SetFont( ftCaption ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
  GUI_DispStringInRect( "工作参数", &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
#endif
  
  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( ListView_x1, ListView_y1, ListView_x2, ListView_y2 );


  GUI_SetColor( crListBkg );
  GUI_FillRect( ConfigList_x1, ConfigList_y1, ConfigList_x2, ConfigList_y2 );  

  osMutexRelease( FGUIState.FLCDMutex ); 
}
//-----------------------------------------------------------------------------
static void _DrawItemCaption(uint32_t uItemIndex)
{

  short Item_y1 = ConfigList_y1 + HEIGHT_ITEM_MAGIN + 
                (uItemIndex - m_ListState.FTopItem) * HEIGHT_MENUITEM,
      Item_y2 = Item_y1 + HEIGHT_MENUITEM - 1;
  if( Item_y2 > ConfigList_y2 - WIDTH_EDGE )
    Item_y2 = ConfigList_y2 - WIDTH_EDGE;
      
  // Erase Item background
  if( m_ListState.FCurItem != uItemIndex )
    {
    GUI_SetColor( crListBkg );

    GUI_FillRect( ConfigList_x1, Item_y1, ConfigList_x2, Item_y2 );
    }
  else
    {
    GUI_SetColor( crListSelBkg );

    GUI_SetColor( crListBk );
    GUI_SetBkColor( crListBk );
    GUI_DrawGradientV( ConfigList_x1 + WIDTH_EDGE + 4, Item_y1 + 2, 
                       ConfigList_x2 - WIDTH_EDGE - 4, Item_y1 + HEIGHT_MENUITEM / 3, 
                       crListSel0,   crListSel1 );
    GUI_DrawGradientV( ConfigList_x1 + WIDTH_EDGE + 4, Item_y1 + HEIGHT_MENUITEM / 3 + 1, 
                       ConfigList_x2 - WIDTH_EDGE - 4, Item_y2 - 2,
                       crListSel2, crListSel3 );
    GUI_SetColor( crListSelEdg );
    GUI_DrawRoundedFrame( ConfigList_x1 + WIDTH_EDGE, Item_y1, 
                          ConfigList_x2 - WIDTH_EDGE, Item_y2, 2, 1 );
    }

      
  // Item Caption
  if( m_ListState.FCurItem != uItemIndex )
    GUI_SetColor( crItemCaption );
  else
    GUI_SetColor( crItemCapSel );
    
  GUI_SetFont ( ftItemCaption );
    
  GUI_RECT Rect = { ItemCaption_x1, Item_y1, ItemCaption_x2, Item_y2 };
  GUI_DispStringInRect( ConfigItems[uItemIndex].Caption, &Rect, 
                        GUI_TA_RIGHT | GUI_TA_VCENTER);
  
  // Item Tail
  Rect.x0 = ItemTail_x1 + WIDTH_EDGE;
  Rect.x1 = ItemTail_x2;
  GUI_DispStringInRect( ConfigItems[uItemIndex].Tail, &Rect, 
                        GUI_TA_LEFT | GUI_TA_VCENTER);
}
//-----------------------------------------------------------------------------
static void _DrawItemValue(uint32_t uItemIndex)
{
  
  if( uItemIndex < m_ListState.FTopItem || 
      m_ListState.FTopItem + VISIBLE_ITEMS < uItemIndex )
    return ;  

  short Item_y1 = ConfigList_y1 + HEIGHT_ITEM_MAGIN + 
                (uItemIndex - m_ListState.FTopItem) * HEIGHT_MENUITEM,
      Item_y2 = Item_y1 + HEIGHT_MENUITEM - 1;
  
  if( m_ListState.FCurItem == uItemIndex )
    {
    if( m_ListState.FEditing != 0 )
      {
      GUI_SetColor( crSprit );
      GUI_DrawRect( ItemEditor_x1, Item_y1, 
                    ItemEditor_x2, Item_y2);

      GUI_SetColor( crEditorBkg );
      GUI_FillRect( ItemEditor_x1 + WIDTH_EDGE / 2, Item_y1 + 1, 
                    ItemEditor_x2 - WIDTH_EDGE / 2, Item_y2 - 1);

      // Text Color
      if( 0 == (m_ListState.FTick & 1) )
        GUI_SetColor( crItemValueSel1 );
      else
        GUI_SetColor( crItemValueSel2 );
      }
    else
      {
//      GUI_SetColor( crListSelBkg );
//        
//      GUI_FillRect( ItemEditor_x1 + WIDTH_EDGE / 2, Item_y1 - HEIGHT_ITEM_MAGIN + 1, 
//                    ItemEditor_x2 - WIDTH_EDGE / 2, Item_y2 + HEIGHT_ITEM_MAGIN - 1);

//      GUI_SetColor( crListBk );
//      GUI_SetBkColor( crListBk );
//      GUI_DrawGradientV( ItemEditor_x1, Item_y1 + 2, 
//                         ItemEditor_x2, Item_y1 + HEIGHT_MENUITEM / 3, 
//                         crListSel0,   crListSel1 );
//      GUI_DrawGradientV( ItemEditor_x1, Item_y1 + HEIGHT_MENUITEM / 3 + 1, 
//                         ItemEditor_x2, Item_y2 - 2,
//                         crListSel2, crListSel3 );
//      GUI_SetColor( crListSelEdg );
//      GUI_DrawLine( ItemEditor_x1, Item_y1, ItemEditor_x2, Item_y1 ); 
//      GUI_DrawLine( ItemEditor_x1, Item_y2, ItemEditor_x2, Item_y2 ); 
   
      // Text Color
      GUI_SetColor( crItemCapSel );
      }
    }
  else
    {
    GUI_SetColor( crListBkg );

    GUI_DrawRect( ItemEditor_x1, Item_y1 - HEIGHT_ITEM_MAGIN, 
                  ItemEditor_x2, Item_y2 + HEIGHT_ITEM_MAGIN );

    // Text Color
    GUI_SetColor( crItemCaption );
    }

  // Value Text
  GUI_SetFont ( ftItemValue );

  short wY1 = Item_y1 - HEIGHT_ITEM_MAGIN,
        wY2 = Item_y2 + HEIGHT_ITEM_MAGIN;
  GUI_RECT Rect = { ItemEditor_x1 + WIDTH_EDGE, wY1, 
                    ItemEditor_x2 - WIDTH_EDGE, wY2 };
  char szStr[16];
  if( 0 == m_ListState.FEditing )
    sprintf( szStr, "%d", GetRegisterValue(ConfigItems[uItemIndex].Register) );
  else
    sprintf( szStr, "%d", m_ListState.FEditValue );
    
  GUI_DispStringInRect( szStr, &Rect, 
                        GUI_TA_RIGHT | GUI_TA_VCENTER);
  
}
//-----------------------------------------------------------------------------
static void _DrawItem(uint32_t uItemIndex)
{
  
  if( uItemIndex < m_ListState.FTopItem || 
      m_ListState.FTopItem + VISIBLE_ITEMS < uItemIndex )
    return ;  
    
  _DrawItemCaption( uItemIndex );
  
  _DrawItemValue( uItemIndex );
}
//-----------------------------------------------------------------------------
static void _UpdateList()
{
  
  osMutexWait( FGUIState.FLCDMutex, osWaitForever );
  
  for( uint32_t uIdx = 0; uIdx < NUM_ConfigItems; uIdx++ )
    {
    _DrawItem( m_ListState.FTopItem + uIdx );
    }

  osMutexRelease( FGUIState.FLCDMutex ); 
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_ListState, 0, sizeof(m_ListState) );
  m_ListState.FCount = NUM_ConfigItems;
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{
  
  _FlushForm();
  
  _UpdateList();
  
  // Next Tick
  m_ListState.FNextTick = GUI_GetTime() + 500;
  
  m_ListState.FTick = 0;
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{

  uTick = GUI_GetTime();
  if( uTick > m_ListState.FNextTick )
    {
    m_ListState.FNextTick =  uTick+ 500;
    
    m_ListState.FTick++;
      
    if( 0 != m_ListState.FEditing )
      _DrawItemValue( m_ListState.FCurItem );
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{

  uint8_t ucCur;
  
  switch( uwKey )
    {
    case KEY_UP:
      {
      if( 0 != m_ListState.FEditing )
        break;
      else if( m_ListState.FCurItem > m_ListState.FTopItem )
        {
         ucCur = m_ListState.FCurItem;
        m_ListState.FCurItem--;
        
        _DrawItem( ucCur );
        _DrawItem( m_ListState.FCurItem );
        }
      else if( m_ListState.FTopItem > 0 )
        {
        m_ListState.FTopItem--;
        m_ListState.FCurItem--;
        _UpdateList();
        }
      
      break;
      }

    case KEY_DOWN:
      {
      if( 0 != m_ListState.FEditing )
        break;
      else if( m_ListState.FCurItem < m_ListState.FCount - 1 )
        {
        if( m_ListState.FCurItem + 1 < m_ListState.FTopItem + VISIBLE_ITEMS)
          {
          ucCur = m_ListState.FCurItem;
          m_ListState.FCurItem++;
          
          _DrawItem( ucCur );
          _DrawItem( m_ListState.FCurItem );
          }
        else
          {
          m_ListState.FCurItem++;
          m_ListState.FTopItem++;
          
          _UpdateList();
          }
        }
      break;
      }

    case KEY_ADD:
      {
      if( 0 == m_ListState.FEditing )
        break;
      else
        {
        const GConfigItem *pItem = &ConfigItems[m_ListState.FCurItem];
        if( pItem->Max >= m_ListState.FEditValue + pItem->Step )
          {
          m_ListState.FEditValue += pItem->Step;
          
          _DrawItemValue( m_ListState.FCurItem );
          }
        }
      break;
      }

    case KEY_SUB:
      {
      if( 0 == m_ListState.FEditing )
        break;
      else
        {
        const GConfigItem *pItem = &ConfigItems[m_ListState.FCurItem];
        if( pItem->Min <= m_ListState.FEditValue - pItem->Step )
          {
          m_ListState.FEditValue -= pItem->Step;
          
          _DrawItemValue( m_ListState.FCurItem );
          }
        }
      break;
      }
      
    case KEY_ENTER:
      {
      const GConfigItem *pItem = &ConfigItems[m_ListState.FCurItem];
      if( 0 == m_ListState.FEditing )
        {
        m_ListState.FEditing = 0x5A;
        
        m_ListState.FEditValue = GetRegisterValue(  pItem->Register );
        }
      else
        {
        SetRegisterValue( pItem->Register, m_ListState.FEditValue );
        if( 0 == FIX_SaveDevConfig( 1 ) )
          memcpy( &DevConfig, &DevCfgForEdit, sizeof(DevConfig) );
          
        m_ListState.FEditing = 0;
        
        _DrawItemCaption( m_ListState.FCurItem );
        }
        
      _DrawItemValue( m_ListState.FCurItem );
        
      break;
      }

    case KEY_ESCAPE:
    case KEY_RETURN:
    case KEY_BACKSPACE:
      {
      // 
      if( 0 == m_ListState.FEditing )
        GUIFormSwitch( WID_MenuForm, 0 );
      else
        {
        m_ListState.FEditing = 0;
        
        _DrawItemCaption( m_ListState.FCurItem );
        _DrawItemValue( m_ListState.FCurItem );
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
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FConfigForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
