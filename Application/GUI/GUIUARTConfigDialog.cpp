//-----------------------------------------------------------------------------
/*
 File        : GUIUARTConfigDIalog.cpp
 Version     : V1.10
 By          : 银网科技
 Description : 自控特性配置窗体

 Date        : 2024.3.5
*/
//-----------------------------------------------------------------------------
#include "GUIUARTConfigDIalog.h"

#include "GUI.h"
#include "GUIConf.h"
#include "GUICntr.h"
#include "FontCHS24LTH.h"

#include "GUIEditors.h"
#include "GUIBitMap.h"
#include "PictureRes.h"
#include "Strings/TextStrs.h"
#include "GLoginDialog.h"

#include "RamHeap.h"

#include "DevRegs.h"
#include "DevFixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION       48

#define WIDTH_EDGE          4
//-----------------------------------------------------------------------------
#define Caption_x1      0
#define Caption_y1      0
#define Caption_x2      (DESKTOP_WIDTH - 1)
#define Caption_y2      (Caption_y1 + HIGHT_CAPTION - 1)

#define CapLabel_x1     (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1     (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2     (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2     (Caption_y2 - WIDTH_EDGE)

//
#define Dialog_x1       (0 + WIDTH_EDGE)
#define Dialog_y1       (Caption_y2 + WIDTH_EDGE)
#define Dialog_x2       (DESKTOP_WIDTH  - WIDTH_EDGE - 1)
#define Dialog_y2       (DESKTOP_HEIGHT - WIDTH_EDGE - 1)

#define EdtPanel_x1     (Dialog_x1 +  WIDTH_EDGE)
#define EdtPanel_y1     (Dialog_y1 +  WIDTH_EDGE)
#define EdtPanel_x2     (Dialog_x2 -  WIDTH_EDGE)
#define EdtPanel_y2     (Dialog_y2 -  WIDTH_EDGE)

// Dialog Colors
#define crFormBkg       GUI_MAKE_COLOR(0x001F1F1F)  
#define crFrameHigh     GUI_MAKE_COLOR(0x002F2F2F)  
#define crFrameDrak     GUI_MAKE_COLOR(0x000F0F0F)

#define crCaptionFont   GUI_GRAY_9A  
#define ftCaption       GUI_FONT_24LTH_CHN

#define crPanelBkg      GUI_MAKE_COLOR(0x00181818)

// Editor
#define WIDTH_LABEL       128
#define WIDTH_LABEL_MARG    4
#define WIDTH_EDITOR      100
#define HEIGHT_ITEM        40
#define HEIGHT_EDITOR      32

#define Lable_x1        (EdtPanel_x1 + 8)
#define Lable_x2        (Lable_x1 + WIDTH_LABEL)

#define Editor_x1       (Lable_x2  + WIDTH_LABEL_MARG + WIDTH_EDGE)
#define Editor_x2       (Editor_x1 + WIDTH_EDITOR)

#define Dime_x1         (Editor_x2   + WIDTH_EDGE * 2)
#define Dime_x2         (EdtPanel_x2 - WIDTH_EDGE)

#define Editor_dy       ((HEIGHT_ITEM - HEIGHT_EDITOR) / 2)
#define Editor_y1(n)    (EdtPanel_y1  + WIDTH_EDGE + (WIDTH_EDGE + HEIGHT_ITEM) * (n))
#define Editor_y2(n)    (Editor_y1(n) + HEIGHT_ITEM - 1)

// Label
#define crLabel         GUI_WHITE
#define ftLabel         GUI_FONT_24LTH_CHN

// Editor
#define crEditorBkgNor  GUI_GRAY_3F
#define crEditorBkgFoc  GUI_DARKBLUE
#define crEditorBdrNor  GUI_GRAY_55 //GUI_MAKE_COLOR(0x00400000) 
#define crEditorBdrFoc  GUI_LIGHTCYAN
#define crEditorInfNor  GUI_WHITE
#define crEditorInfFoc  GUI_ORANGE 
#define ftEditor        GUI_FONT_24LTH_CHN

// Button
#define Button_Width    100
#define Button_Height   36

#define Button_x1       ((DESKTOP_WIDTH) / 2 - Button_Width - 10)
#define Button_x2       ((DESKTOP_WIDTH) / 2 + 10)
#define Button_y1       (Button_y2 - Button_Height + 1)
#define Button_y2       (Dialog_y2 - WIDTH_EDGE - 1)

#define rectButton1     { Button_x1,                    Button_y1, \
                          Button_x1 + Button_Width - 1, Button_y2 }
#define rectButton2     { Button_x2,                    Button_y1, \
                          Button_x2 + Button_Width - 1, Button_y2 }

#define crButtonBkgNor  GUI_GRAY_3F
#define crButtonBkgFoc  GUI_DARKBLUE
#define crButtonBdrNor  GUI_GRAY_3F
#define crButtonBdrFoc  GUI_LIGHTCYAN
#define crButtonInfNor  GUI_GRAY_C0
#define crButtonInfFoc  GUI_WHITE 
#define ftButton        GUI_FONT_24LTH_CHN

//-----------------------------------------------------------------------------
#define DISP_LABEL_TEXT ftLabel, crLabel, GUI_TA_RIGHT | GUI_TA_VCENTER
#define DISP_DIM_TEXT   ftLabel, crLabel, GUI_TA_LEFT  | GUI_TA_VCENTER

#define DISP_LABEL_NONE   { {0}, nullptr, 0, 0, 0 }

#define LabelDef(n, s) { { Lable_x1, Editor_y1(n), Lable_x2, Editor_y2(n) }, \
                         DISP_LABEL_TEXT, (s) }
#define DimDef(n, s)   { { Dime_x1,  Editor_y1(n), Dime_x2,  Editor_y2(n) }, \
                         DISP_DIM_TEXT,   (s) }

#define DISP_LABEL_BTN(s) { {0}, nullptr, 0, 0, s }

#define EditorRect(n) { Editor_x1,  Editor_y1(n) + Editor_dy,  \
                        Editor_x2,  Editor_y2(n) - Editor_dy }

#define STYLE_EDITOR  ftEditor, GUI_TA_RIGHT | GUI_TA_VCENTER, \
        { crEditorBkgNor, crEditorBdrNor, crEditorInfNor },    \
        { crEditorBkgFoc, crEditorBdrFoc, crEditorInfFoc }

#define STYLE_BUTTON  ftButton, GUI_TA_CENTER | GUI_TA_VCENTER, \
        { crButtonBkgNor, crButtonBdrNor, crButtonInfNor },     \
        { crButtonBkgFoc, crButtonBdrFoc, crButtonInfFoc }
//=============================================================================
// 本地静态数据
//-----------------------------------------------------------------------------
static constexpr uint32_t plstBaud[] =
{
  idUartBaud01, idUartBaud02, idUartBaud03, idUartBaud04, idUartBaud05
};
#define Num_plstParity  (sizeof(plstParity) / sizeof(plstParity[0]))
//-----------------------------------------------------------------------------
static constexpr uint32_t plstParity[] =
{
  idUartParity0, idUartParity1, idUartParity2
};
#define Num_plstBaud  (sizeof(plstBaud) / sizeof(plstBaud[0]))
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static constexpr TEditorObj  cEditorDefs[] =
{
  //---- Stopbits
  { 
    LabelDef( 0, idUARTCfgName02), // Caption
    DISP_LABEL_NONE,     // Dim
    EditorRect(0),       // Editor rect 
    STYLE_EDITOR,
    EDT_NUMBER,          // uType
    1,                   // uMin
    252,                 // uMax
    nullptr              // pList
  }
  //---- Baudrate
 ,{ 
    LabelDef( 1, idUARTCfgName03), // Caption
    DimDef(   1, idUartUnit01),
    EditorRect(1),       // Editor rect 
    STYLE_EDITOR,
    EDT_LIST,            // uType
    0,                   // uMin
    Num_plstBaud - 1,    // uMax
    plstBaud             // pList
  }

  //---- Parity
 ,{ 
    LabelDef( 2, idUARTCfgName04), // Caption
    DISP_LABEL_NONE,     // Dim
    EditorRect(2),       // Editor rect 
    STYLE_EDITOR,
    EDT_LIST,            // uType
    0,                   // uMin
    Num_plstParity - 1,  // uMax
    plstParity           // pList
  }

  //---- Buttom-Ok
 ,{ 
    DISP_LABEL_BTN(idBtnCapOk),     // Caption
    DISP_LABEL_NONE,     // Dim
    rectButton1,         // Button rect 
    STYLE_BUTTON,
    EDT_BUTTON,          // uType
    GM_EDITOR_ACCEPT,    // Message Ident
    0,                   // Message Ident
    nullptr              // pList
  }

  //---- Buttom-Cancel
 ,{ 
    DISP_LABEL_BTN(idBtnCapCancel), // Caption
    DISP_LABEL_NONE,     // Dim
    rectButton2,         // Button rect 
    STYLE_BUTTON,
    EDT_BUTTON,          // uType
    GM_EDITOR_CANCEL,    // Message Ident
    0,                   // Message Value
    nullptr              // pList
  }
};
#define NUM_Editors      (sizeof(cEditorDefs) / sizeof(cEditorDefs[0]))
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 对话框状态数据
typedef struct tagDialogState
{

  uint16_t     uwCurEditorIdx;
  uint16_t     uwEditorCount;
  
  uint32_t     uTickCount;
  uint32_t     uNextTick;
  
  TEditor     *pEditors[NUM_Editors];

  const GWinForm *pDialog;
} TDialogState;
//-----------------------------------------------------------------------------
// 对话框状态数据
static TDialogState  m_DialogState = {0};
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#ifndef size_t
  #define size_t unsigned int
#endif
inline void* operator new(size_t, void* ptr )
{  return ptr; }
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
static void _buildEditors()
{

  const TUARTConfig *pConfig = GET_UARTOPT(0);
  for( uint32_t uIdx = 0; uIdx < NUM_Editors; uIdx++ )
    {
    void* pMem = RAM_Malloc( sizeof(TEditor) );
#ifdef USE_DEV_ASSERT
     DEV_ASSERT( nullptr == pMem, GFC_OutOfMem );
#endif
    TEditor *pEditor = new (pMem)TEditor(cEditorDefs + uIdx);
#ifdef USE_DEV_ASSERT
     DEV_ASSERT( nullptr == pEditor, GFC_OutOfMem );
#else
    if( nullptr == pEditor )
      return;
#endif
    m_DialogState.pEditors[uIdx] = pEditor;

    switch( uIdx )
      {
      case 0:  // Address
        {
        pEditor->setValue(pConfig->Addr);
        break;
        }
      case 1:  // Baudrate
        {
        pEditor->setValue(pConfig->Baudrate);
        break;
        }
      case 2:  // Parity
        {
        uint32_t uParity = pConfig->Parity;
        pEditor->setValue( uParity );
        break;
        }
//      case 3:  // Stopbits
//        {
//        uint32_t uStopbits = (pConfig->Config & UCFG_STOPBITS_MASK)? 2 : 1;
//        pEditor->setValue( uStopbits );
//        break;
//        }
//      case 3:  // Length
//        {
//        uint32_t uLength = pConfig->Length;
//        pEditor->setValue( uLength );
//        break;
//        }
//      case 4:  // Protocol
//        {
//        uint32_t uProtocol = pConfig->Protocol;
//        pEditor->setValue( uProtocol );
//        break;
//        }
//      case 5:  // Interval
//        {
//        uint32_t uInterval = pConfig->Interval;
//        pEditor->setValue( uInterval );
//        break;
//        }
      default:
        {
        pEditor->setValue( 0 );
        break;
        }
      }
    }
}
//-----------------------------------------------------------------------------
static void _acceptEditors()
{

  TUARTConfig *pConfig = &DevCfgForEdit.UARTConfig[0];
  
  // Address
  pConfig->Addr = m_DialogState.pEditors[0]->getValue();

  // Baudrate
  pConfig->Baudrate = m_DialogState.pEditors[1]->getValue();

  // Parity
  pConfig->Parity = m_DialogState.pEditors[2]->getValue();
  
  // Wey. 2023.8.18
  // 复制到运行区
  memcpy( (uint8_t*)&DevConfig, &DevCfgForEdit, sizeof(DevConfig) );

  // 需要保存
  SetTodoTask( RTT_SAVE_DCFG );

  // 需要配置硬件
  SetSetHW(RSH_UART1);
}
//-----------------------------------------------------------------------------
static void _showForm()
{
  
  osMutexWait( FGUIState.mutexGUI, osWaitForever );
  
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_RECT Rect1 = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
  GUI_DrawRectEx( &Rect1 );

  const char* pStr = GetMultiLangString(idUARTCfgName01);
  if( nullptr != pStr )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
    GUI_DispStringInRect( pStr, &Rect1, GUI_TA_CENTER | GUI_TA_VCENTER );
    }
#endif

  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( Dialog_x1, Dialog_y1, Dialog_x2, Dialog_y2 );

  GUI_SetColor( crPanelBkg );
  GUI_FillRect( EdtPanel_x1, EdtPanel_y1, EdtPanel_x2, EdtPanel_y2 );

  // 所有编辑器重绘
  for( uint32_t uIdx = 0; uIdx < NUM_Editors; uIdx++ )
    {
    m_DialogState.pEditors[uIdx]->repint( true );
    }
  m_DialogState.pEditors[m_DialogState.uwCurEditorIdx]->setFocused( true );

  osMutexRelease( FGUIState.mutexGUI ); 
}
//=============================================================================
// 接口方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{
  
  memset( &m_DialogState, 0, sizeof(m_DialogState) );
  m_DialogState.uwEditorCount  = NUM_Editors;
  m_DialogState.uwCurEditorIdx = 0;
  
  _buildEditors();
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{

  _showForm();

  // Next Tick
  m_DialogState.uNextTick = GUI_GetTime() + 500;

  m_DialogState.uTickCount = 0;
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{

  for( uint32_t uIdx = 0; uIdx < NUM_Editors; uIdx++ )
    {
    RAM_Free( m_DialogState.pEditors[uIdx] );
    }
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{

  uTick = GUI_GetTime();
  if( uTick > m_DialogState.uNextTick )
    {
    m_DialogState.uNextTick = uTick+ 500;
    
    m_DialogState.uTickCount++;
      
    TEditor *pEditor = m_DialogState.pEditors[m_DialogState.uwCurEditorIdx];
    pEditor->onTick( uTick );
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyDown( uint16_t uwKey, uint32_t uPress )
{

  TEditor* pCurEditor = m_DialogState.pEditors[ m_DialogState.uwCurEditorIdx ];
  
  if( true == pCurEditor->getEditing() )
    {
    pCurEditor->onKeyDown(uwKey, uPress);
      
    return ;
    }
  
  switch( uwKey )
    {
    case KEY_UP:
      {
      uint32_t uNewIdx = m_DialogState.uwCurEditorIdx;
      if( 0 < uNewIdx )
        uNewIdx--;
      else
        uNewIdx = m_DialogState.uwEditorCount - 1;

      pCurEditor->setFocused( false );
      m_DialogState.pEditors[uNewIdx]->setFocused( true );

      m_DialogState.uwCurEditorIdx = uNewIdx;

      break;
      }

    case KEY_DOWN:
      {
      uint32_t uNewIdx = m_DialogState.uwCurEditorIdx;
      if( m_DialogState.uwEditorCount > uNewIdx + 1 )
        uNewIdx++;
      else
        uNewIdx = 0;

      pCurEditor->setFocused( false );
      m_DialogState.pEditors[uNewIdx]->setFocused( true );

      m_DialogState.uwCurEditorIdx = uNewIdx;

      break;
      }

    case KEY_ENTER:
      {
      if( EDT_BUTTON == cEditorDefs[m_DialogState.uwCurEditorIdx].uType )
        pCurEditor->onKeyDown(uwKey, uPress);
      else if( false == pCurEditor->getEditing() )
        {
        pCurEditor->setEditing( true );
        }

      break;
      }

//    case KEY_ESCAPE:
//      {
//      GUIFormSwitch( WID_MenuForm, (void*)2 );

//      break;
//      }

    default:
      {
      pCurEditor->onKeyDown(uwKey);
      break;
      }      
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp( uint16_t uwKey, uint32_t uPress )
{

  TEditor* pCurEditor = m_DialogState.pEditors[ m_DialogState.uwCurEditorIdx ];
  
  if( true == pCurEditor->getEditing() )
    {
    pCurEditor->onKeyDown(uwKey, uPress);
      
    return ;
    }
  
  switch( uwKey )
    {
    case KEY_ESCAPE:
      {
      GUIFormClose(nullptr );

      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg )
{
  
  if( nullptr == pMsg )
    return ;

  
//  if( 0 == m_DialogState.uwEditorCount )
//    {
//    GUIFormSwitch( WID_MenuForm, (void*)2 );
//    
//    pMsg->MsgId = 0;
//      
//    return ;
//    }

  if( nullptr != m_DialogState.pDialog )
    m_DialogState.pDialog->pMsg( pMsg );

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
        _OnKeyDown( pMsg->Param, pMsg->Data.v );
        
        pMsg->MsgId = 0;
        }
        
      break;
      }

    case GM_KEYUP:
      {
      if( pMsg->Param )
        {
        _OnKeyUp( pMsg->Param, pMsg->Data.v );
        
        pMsg->MsgId = 0;
        }
        
      break;
      }

    case GM_EDITOR_ACCEPT:
      {
      if( nullptr == m_DialogState.pDialog )
        {
        m_DialogState.pDialog = &FLoginDialog;
        
        m_DialogState.pDialog->pInit( nullptr );
        m_DialogState.pDialog->pShow( nullptr );
        }
      else if( &FLoginDialog == pMsg->Data.p )
        {
        if( pMsg->Param == DevConfig.Password )
          _acceptEditors();
        
        m_DialogState.pDialog->pClose( nullptr );
        m_DialogState.pDialog = nullptr;

//        GUIFormSwitch( WID_MenuForm, (void*)2 );
        m_DialogState.uwEditorCount = 0;
        }

      pMsg->MsgId = 0;
        
      break;
      }

    case GM_EDITOR_CANCEL:
      {
      if( nullptr != pMsg->Data.p )
        {
        m_DialogState.pDialog->pClose( nullptr );
        m_DialogState.pDialog = nullptr;
        }

//      GUIFormSwitch( WID_MenuForm, (void*)2 );
      m_DialogState.uwEditorCount = 0;

      pMsg->MsgId = 0;
        
      break;
      }
    }
}
//=============================================================================
// 窗体接口
//-----------------------------------------------------------------------------
const GWinForm FUARTConfigForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
