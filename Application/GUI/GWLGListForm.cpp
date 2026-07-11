//-----------------------------------------------------------------------------
/*
 File        : GPWLGListForm.cpp
 Version     : V1.10
 By          : 银网科技
 Description: 波形记录列表窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GWLGListForm.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "GUIFontCHS12x12.h"
#include "FontCHS24x24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"

#include "WaveLogger.h"
#include "GWLGViewForm.h"

#include <stdlib.h>
#include <string.h>
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// Menu Command
#define MC_NONE         0x00
#define MC_OPENFORM     0x01
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     40

#define WIDTH_EDGE        4

#define HEIGHT_LISTITEM   28

#define VISIBLE_ITEMS     (uint16_t)((WavelogList_H - WIDTH_EDGE * 2) / HEIGHT_LISTITEM)
//-----------------------------------------------------------------------------
#define Caption_x1      0
#define Caption_y1      0
#define Caption_x2      (DESKTOP_WIDTH - 1)
#define Caption_y2      (Caption_x1 + HIGHT_CAPTION - 1)

#define CapLabel_x1     (Caption_x1 +  WIDTH_EDGE)
#define CapLabel_y1     (Caption_y1 +  WIDTH_EDGE)
#define CapLabel_x2     (Caption_x2 -  WIDTH_EDGE)
#define CapLabel_y2     (Caption_y2 -  WIDTH_EDGE)

// GuageBar
#define GuageBar_x1     (GuageBar_x2 - 5)
#define GuageBar_y1     (ListView_y1 + WIDTH_EDGE)
#define GuageBar_x2     (ListView_x2 - WIDTH_EDGE)
#define GuageBar_y2     (ListView_y2 - WIDTH_EDGE)
#define GuageBar_HEIGHT (GuageBar_y2 - GuageBar_y1 + 1)

// Wavelog List
#define ListView_x1     (0 + WIDTH_EDGE)
#define ListView_y1     (Caption_y2 + WIDTH_EDGE)
#define ListView_x2     (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define ListView_y2     (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define WavelogList_x1     (ListView_x1 +  WIDTH_EDGE)
#define WavelogList_y1     (ListView_y1 +  WIDTH_EDGE)
#define WavelogList_x2     (GuageBar_x2 -  WIDTH_EDGE)
#define WavelogList_y2     (ListView_y2 -  WIDTH_EDGE)
#define WavelogItem_x1     (WavelogList_x1 + WIDTH_EDGE * 8)
#define WavelogItem_x2     (WavelogList_x2 - WIDTH_EDGE * 8)

#define WavelogList_W      (WavelogList_x2 - WavelogList_x1 + 1)
#define WavelogList_H      (WavelogList_y2 - WavelogList_y1 + 1)

// Colors
#define crFormBkg       0x001F1F1F  
#define crFrameHigh     0x002F2F2F  
#define crFrameDrak     0x000F0F0F

#define crCaptionFont   GUI_GRAY_9A  
#define ftCaption      &GUI_CHSLTHFont24

#define ftItemCaption   &GUI_CHSLTHFont24
#define crItemCaption   GUI_CYAN 
#define crItemCapSel    0x003000  //GUI_LIGHTGREEN 

#define crSprit         GUI_GRAY_7C

#define crListBkg       0x181818 
#define crListSel0      0xD0DFFD  // r253, g239, b224
#define crListSel1      0xBCCFFE  // r253, g223, b188
#define crListSel2      0x69CEFF  // r255, g206, b105
#define crListSel3      0x99D4FF  // r255, g228, b153
#define crListSelEdg    0x00B7FF  // 

#define crGuageBarBkg   0x3F3F3F  //
#define crGuageBar      0xA0A03F  //
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 列表状态数据
typedef struct __GWavelogList
{

  uint16_t    FTopItem;
  uint16_t    FCurItem;
  
  uint16_t    FTick;
  uint32_t    FNextTick;
  uint16_t    FCount;
  
  // 摘要存区
  TEventLogSummary m_SummaryCache[VISIBLE_ITEMS + 1];
  
  const GWinForm *FDetailForm;
} GWavelogList;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 列表状态
static GWavelogList m_FormState = {0};
//-----------------------------------------------------------------------------

//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//---------------------------------------------------------------------------
// 取列表项目数量
// 输入：(无)
// 输出：(无)
// 返回：
//   列表中的项目数量
static uint16_t _GetItemCount(void)
{

  uint16_t uwItemCnt = WAVLOG_GetLogCount();

  return uwItemCnt;
}
//-----------------------------------------------------------------------------
static void _DrawGuageBar()
{

  //   
  GUI_SetColor( crGuageBarBkg );
  GUI_FillRect( GuageBar_x1,  GuageBar_y1,  GuageBar_x2,  GuageBar_y2 );
  
  if( m_FormState.FCount > 0 )
    {
    uint16_t uIdx = m_FormState.FCurItem;
    uint32_t uBarBegin = GuageBar_y1 + 
                         uIdx * GuageBar_HEIGHT / m_FormState.FCount,
             uBarEnd   = GuageBar_y1 + 
                         (uIdx + 1) * GuageBar_HEIGHT / m_FormState.FCount;
    if( uBarEnd > GuageBar_y2 )
      uBarEnd = GuageBar_y2;

    // Slid Bar
    GUI_SetColor( crGuageBar );
    GUI_FillRect( GuageBar_x1, uBarBegin, GuageBar_x2, uBarEnd );
    }
}
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
  
  GUI_DispStringInRect( "波形记录", &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
#endif
  
  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( ListView_x1, ListView_y1, ListView_x2, ListView_y2 );


  GUI_SetColor( crListBkg );
  GUI_FillRect( WavelogList_x1, WavelogList_y1, WavelogList_x2, WavelogList_y2 );  

  if( m_FormState.FCount < 1 )
    {
    GUI_SetColor( crItemCaption );
    GUI_SetFont ( ftItemCaption );
      
    GUI_RECT Rect = { WavelogList_x1, WavelogList_y1, WavelogList_x2, WavelogList_y2 };
    GUI_DispStringInRect( "无波形记录", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER);
    }
    
  osMutexRelease( FGUIState.FLCDMutex ); 
}
//-----------------------------------------------------------------------------
static void _DrawItemCaption(uint32_t uItemIndex)
{
  
  int Item_x1 = WavelogList_x1 + WIDTH_EDGE,
      Item_x2 = WavelogList_x2 - WIDTH_EDGE,
      Item_y1 = WavelogList_y1 + WIDTH_EDGE + 
                (uItemIndex - m_FormState.FTopItem) * HEIGHT_LISTITEM,
      Item_y2 = Item_y1 + HEIGHT_LISTITEM - 1;
  if( Item_y2 > WavelogList_y2 - WIDTH_EDGE )
    Item_y2 = WavelogList_y2 - WIDTH_EDGE;
      
  if( m_FormState.FCurItem != uItemIndex )
    {
    GUI_SetColor( crListBkg );
    GUI_FillRect( Item_x1, Item_y1, Item_x2, Item_y2 );
    }
  else
    {
    GUI_SetColor( crListBkg );
    GUI_SetBkColor( crListBkg );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + 2, 
                     Item_x2 - 4, Item_y1 + HEIGHT_LISTITEM / 3, 
                     crListSel0,   crListSel1 );
    GUI_DrawGradientV( Item_x1 + 4, Item_y1 + HEIGHT_LISTITEM / 3 + 1, 
                     Item_x2 - 4, Item_y2 - 2,
                     crListSel2, crListSel3 );
    GUI_SetColor( crListSelEdg );
    GUI_DrawRoundedFrame( Item_x1, Item_y1, 
                            Item_x2, Item_y2, 2, 1 );
    }
        
  if( m_FormState.FCurItem != uItemIndex )
    GUI_SetColor( crItemCaption );
  else
    GUI_SetColor( crItemCapSel );
    
  GUI_SetFont ( ftItemCaption );
    
  // 从缓冲区中取事件记录项
  TEventLogSummary* pWavelog;
  if( uItemIndex - m_FormState.FTopItem < VISIBLE_ITEMS )
    pWavelog = &(m_FormState.m_SummaryCache[uItemIndex - m_FormState.FTopItem]);
  else
    pWavelog = 0;
  
  char szText[64];
  sprintf( szText,
           "%04u-%02u-%02u %02u:%02u:%02u:%03u",
           pWavelog->Time.Year,
           pWavelog->Time.Month,
           pWavelog->Time.Day,
           pWavelog->Time.Hours,
           pWavelog->Time.Minutes,
           pWavelog->Time.Seconds,
           pWavelog->Time.milSecs );

  GUI_RECT Rect = { WavelogItem_x1, Item_y1, WavelogItem_x2, Item_y2 };
  GUI_DispStringInRect( szText, &Rect,
                        GUI_TA_LEFT | GUI_TA_VCENTER);
}
//-----------------------------------------------------------------------------
static void _DrawItem(uint32_t uItemIndex)
{
  
  if( uItemIndex < m_FormState.FTopItem || 
      m_FormState.FTopItem + VISIBLE_ITEMS <= uItemIndex )
    return ;  
    
  _DrawItemCaption( uItemIndex );
}
//-----------------------------------------------------------------------------
static void _UpdateList()
{
  
  osMutexWait( FGUIState.FLCDMutex, osWaitForever );

  // 清缓冲区
  memset( &m_FormState.m_SummaryCache, 0, sizeof(m_FormState.m_SummaryCache) );
  
  for( uint32_t uIndex = m_FormState.FTopItem; uIndex < m_FormState.FCount; uIndex++ )
    {
    uint32_t uVisIndex = uIndex - m_FormState.FTopItem;
    if( uVisIndex >= VISIBLE_ITEMS )
      break;

    // 读录波记录摘要到缓冲区
    if( WAVLOG_GetLogSummary( &m_FormState.m_SummaryCache[uVisIndex], uIndex ) != 0 )
      m_FormState.m_SummaryCache[uVisIndex].Time.Year = 0;
      
    _DrawItem( uIndex );
    }

  _DrawGuageBar();

  osMutexRelease( FGUIState.FLCDMutex ); 
}

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_FormState, 0, sizeof(m_FormState) );
  m_FormState.FCount = _GetItemCount();
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{

  m_FormState.FCurItem = (uint32_t)argument;
  if( m_FormState.FCount <= m_FormState.FCurItem )
    m_FormState.FCurItem = 0;
  
  _FlushForm();
  
  _UpdateList();
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
static void _OnKeyDown(uint16_t uwKey)
{

  uint8_t ucCur;
  
  switch( uwKey )
    {
    case KEY_ESCAPE:
      GUIFormSwitch( WID_MenuForm, 3 );
      break;
    
    case KEY_UP:
      {
      if( m_FormState.FCurItem > m_FormState.FTopItem )
        {
         ucCur = m_FormState.FCurItem;
        m_FormState.FCurItem--;
        
        _DrawItem( ucCur );
        _DrawItem( m_FormState.FCurItem );
        _DrawGuageBar();
        }
      else if( m_FormState.FTopItem > 0 )
        {
        m_FormState.FTopItem--;
        m_FormState.FCurItem--;
        _UpdateList();
        }
      
      break;
      }

    case KEY_DOWN:
      {
      if( m_FormState.FCurItem < m_FormState.FCount - 1 )
        {
        if( m_FormState.FCurItem + 1 < m_FormState.FTopItem + VISIBLE_ITEMS )
          {
          ucCur = m_FormState.FCurItem;
          m_FormState.FCurItem++;
          
          _DrawItem( ucCur );
          _DrawItem( m_FormState.FCurItem );
          _DrawGuageBar();
          }
        else
          {
          m_FormState.FCurItem++;
          m_FormState.FTopItem++;
          
          _UpdateList();
          }
        }
      break;
      }

    case KEY_ENTER:
      {
      if( m_FormState.FCount > 0 )
        {
        m_FormState.FDetailForm = &FWavelogViewForm;
          
        m_FormState.FDetailForm->pInit( &FWavelogListForm );
        m_FormState.FDetailForm->pShow( m_FormState.FCurItem + m_FormState.FTopItem );
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

  if( 0 != m_FormState.FDetailForm )
    {
    m_FormState.FDetailForm->pMsg( pMsg );
    }
  
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
      
    case GM_EDITOR_ACCEPT:
    case GM_EDITOR_CANCEL:
      {
      if( 0 != m_FormState.FDetailForm )
        m_FormState.FDetailForm->pClose( 0 );
        
      m_FormState.FDetailForm = 0;
        
      _FlushForm();
  
      _UpdateList();
      
      break;
      }
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FWavelogListForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
#endif //WAVELOGGER_EN > 0