//-----------------------------------------------------------------------------
/*
 File        : GEditorPanel.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GEditor + GNumberEditor + GEditorPanel implementation.
               Per SG1210v25 dialog spec sections 3.5 / 3.6 / 3.7.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec;
              includes E-8 focus model and editor switching logic)
*/
//-----------------------------------------------------------------------------
#include "GDialog.h"
#include "GEditorPanel.h"

#include "DevDebug.h"
#include "GUI.h"
#include "RamHeap.h"

#include "GWidget.h"
#include <GUI_Type.h>
#include <cstdio>
#include <cstring>
#include <new>

//=============================================================================
// GEditor
//-----------------------------------------------------------------------------
GEditor::GEditor(GDialog* pOwner, const GStyle* pStyle,
                 const GConfig* pConfig, int x0, int y0)
  : GWidget(reinterpret_cast<GWidget*>(pOwner))
{
  m_pStyle  = pStyle;
  m_pConfig = pConfig;
  if( nullptr != pConfig ) {
    m_Rect.x = pConfig->xEdt + x0;
    m_Rect.y = pConfig->yEdt + y0;
    m_Rect.w = pConfig->wEdt;
    m_Rect.h = pConfig->hEdt;
  }
}

//-----------------------------------------------------------------------------
GEditor::~GEditor()
{
  if( nullptr != m_pText ) {
    RAM_Free(m_pText);
    m_pText = nullptr;
  }
}

//-----------------------------------------------------------------------------
void GEditor::Init()
{
  if( nullptr != m_pFormat ) {
    size_t nLength = strlen(m_pFormat);
    if( 0 < nLength ) {
      m_pText = (char*)RAM_Malloc(nLength + 1);   // +1 for NUL terminator
      if( nullptr != m_pText ) {
        strcpy(m_pText, m_pFormat);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void GEditor::onShow()
{
  DEV_ASSERT(( nullptr == m_pConfig || nullptr == m_pStyle ), GFC_EmptyPtr);

  int x0 = m_Rect.x;
  int y0 = m_Rect.y;
  int x1 = m_Rect.x + m_Rect.w - 1;
  int y1 = m_Rect.y + m_Rect.h - 1;

  // Background (only in NORMAL draw mode)
  if( GUI_DRAWMODE_TRANS != m_pConfig->drawMode ) {
    GUI_SetColor(m_pStyle->crBackground);
    GUI_FillRect(x0, y0, x1, y1);
  }

  // Frame (normal vs focused)
  GUI_SetColor((true == m_bFocused) ? m_pStyle->crFrameFocus : m_pStyle->crFrame);
  GUI_DrawRect(x0, y0, x1, y1);

  // Text
  if( nullptr != m_pText ) {
    if( nullptr != m_pStyle->ftText ) {
      GUI_SetFont(m_pStyle->ftText);
    }
    GUI_SetColor((true == m_bFocused) ? m_pStyle->crTextFocus : m_pStyle->crText);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);

    GUI_RECT rTxt;
    rTxt.x0 = m_Rect.x + m_pConfig->xTxt;
    rTxt.y0 = m_Rect.y + m_pConfig->yTxt;
    rTxt.x1 = m_Rect.x + m_pConfig->xTxt + m_pConfig->wTxt - 1;
    rTxt.y1 = m_Rect.y + m_pConfig->yTxt + m_pConfig->hTxt - 1;
    GUI_DispStringInRect(m_pText, &rTxt, m_pConfig->alignment);
  }

  // Cursor (TODO: XOR flashing cursor implementation)
}

//-----------------------------------------------------------------------------
void GEditor::onTick(uint32_t uTick)
{
  (void)uTick;
  // TODO: flash cursor every 500ms
}

//-----------------------------------------------------------------------------
GEditor::GKeyStatus GEditor::onGKeyPress(uint32_t uKey, uint32_t uRepeat)
{
  (void)uKey;
  (void)uRepeat;
  // Base editor accepts all keys (override in derived class)
  return krAccepted;
}

//=============================================================================
// GNumberEditor
//-----------------------------------------------------------------------------
GNumberEditor::GNumberEditor(GDialog* pOwner, const GStyle* pStyle,
                             const GConfig* pConfig, int epX0, int epY0)
  : GEditor(pOwner, pStyle, pConfig, epX0, epY0)
{
}

//-----------------------------------------------------------------------------
void GNumberEditor::Init()
{
  GEditor::Init();
  if( nullptr != m_pText ) {
    snprintf(m_pText, strlen(m_pFormat) + 1, m_pFormat, m_Value);
  }
}

//-----------------------------------------------------------------------------
void GNumberEditor::onShow()
{
  // Update text before drawing
  if( nullptr != m_pText && nullptr != m_pFormat ) {
    snprintf(m_pText, strlen(m_pFormat) + 1, m_pFormat, m_Value);
  }
  GEditor::onShow();
}

//-----------------------------------------------------------------------------
void GNumberEditor::setRange(const int vMin, const int vMax)
{
  m_vMin = vMin;
  m_vMax = vMax;
}

//-----------------------------------------------------------------------------
void GNumberEditor::setValue(int value)
{
  m_Value = value;
  if( m_vMax < m_Value ) {
    m_Value = m_vMax;
  }
  if( m_Value < m_vMin ) {
    m_Value = m_vMin;
  }
}

//-----------------------------------------------------------------------------
int GNumberEditor::getValue() const
{
  return m_Value;
}

//-----------------------------------------------------------------------------
bool GNumberEditor::validate() const
{
  return (m_vMin <= m_Value && m_Value <= m_vMax);
}

//-----------------------------------------------------------------------------
GEditor::GKeyStatus GNumberEditor::onGKeyPress(uint32_t uKey, uint32_t uRepeat)
{
  (void)uRepeat;

  // Digit keys '0'-'9'
  if( '0' <= uKey && uKey <= '9' ) {
    int digit = uKey - '0';
    int newVal = m_Value * 10 + digit;
    if( newVal <= m_vMax ) {
      m_Value = newVal;
      onShow();
      return krAccepted;
    } else {
      return krRejected;
    }
  }

  // Backspace
  if( KEY_BACKSPACE == uKey ) {
    if( 0 < m_Value ) {
      m_Value /= 10;
      onShow();
      return krAccepted;
    } else {
      return krMovePrev;
    }
  }

  // Arrow keys
  if( KEY_LEFT == uKey ) {
    return krMovePrev;
  }
  if( KEY_RIGHT == uKey ) {
    return krMoveNext;
  }

  return krAccepted;
}

//=============================================================================
// GEditorPanel
//-----------------------------------------------------------------------------
GEditorPanel::GEditorPanel(GDialog* pDialog, const GStyle* pStyle,
                           const GConfig* pConfig, int iCurIndex)
  : GWidget(reinterpret_cast<GWidget*>(pDialog))
{
  m_pStyle   = pStyle;
  m_pConfig  = pConfig;
  m_CurIndex = iCurIndex;
}

//-----------------------------------------------------------------------------
GEditorPanel::~GEditorPanel()
{
  if( nullptr != m_Editors && nullptr != m_pConfig ) {
    for( uint32_t i = 0; i < m_pConfig->Count; i++ ) {
      m_Editors[i].~GEditor();
    }
    RAM_Free(m_Editors);
    m_Editors = nullptr;
  }
}

//-----------------------------------------------------------------------------
void GEditorPanel::Init()
{
  DEV_ASSERT(( nullptr == m_pConfig || nullptr == m_pStyle ), GFC_EmptyPtr);

  uint32_t n = m_pConfig->Count;
  if( 0 == n ) {
    return;
  }

  auto mLen = n * sizeof(GNumberEditor);
  auto ptr  = (uint8_t*)RAM_Malloc(mLen);
  DEV_ASSERT( nullptr == ptr, GFC_OutOfMem );
  m_Editors = (GEditor*)ptr;

  GDialog* pDlg = reinterpret_cast<GDialog*>(m_pParent);
  for( uint32_t index = 0; index < n; index++ ) {
    // Placement new - construct GNumberEditor
    new (ptr) GNumberEditor(pDlg,
                            &(m_pStyle->editorStyle),
                            m_pConfig->Editors + index,
                            m_pConfig->x,
                            m_pConfig->y);
    ptr += sizeof(GNumberEditor);
  }

  // E-8 FIXED: Set initial focus to first editor
  if( 0 < n && nullptr != m_Editors ) {
    m_CurIndex = 0;
    m_Editors[0].setFocus(true);
  }
}

//-----------------------------------------------------------------------------
void GEditorPanel::onShow()
{
  if( nullptr == m_pConfig || nullptr == m_Editors ) {
    return;
  }

  for( uint32_t i = 0; i < m_pConfig->Count; i++ ) {
    m_Editors[i].onShow();
  }
}

//-----------------------------------------------------------------------------
bool GEditorPanel::validate() const
{
  if( nullptr == m_pConfig || nullptr == m_Editors ) {
    return true;
  }

  for( uint32_t index = 0; index < m_pConfig->Count; index++ ) {
    GEditor* editor = m_Editors + index;
    if( false == editor->validate() ) {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// E-8 FIXED: Handle key input with automatic focus switching
void GEditorPanel::onGKeyPress(uint32_t uKey, uint32_t uRepeat)
{
  if( nullptr == m_Editors || 0 == m_pConfig->Count ) {
    return;
  }

  GEditor* pCurEditor = &m_Editors[m_CurIndex];

  // Forward key to current editor and check navigation request
  GEditor::GKeyStatus status = pCurEditor->onGKeyPress(uKey, uRepeat);

  switch( status ) {
    case GEditor::krMovePrev: {
      // Move focus to previous editor
      if( 0 < m_CurIndex ) {
        pCurEditor->setFocus(false);
        m_CurIndex--;
        m_Editors[m_CurIndex].setFocus(true);
      }
      break;
    }
    case GEditor::krMoveNext: {
      // Move focus to next editor
      if( m_CurIndex < m_pConfig->Count - 1 ) {
        pCurEditor->setFocus(false);
        m_CurIndex++;
        m_Editors[m_CurIndex].setFocus(true);
      }
      break;
    }
    case GEditor::krAccepted:
    case GEditor::krRejected:
      // Key processed, no focus change
      break;
    default:
      // krMoveUp/Down for multi-line panels - not implemented yet
      break;
  }
}
//-----------------------------------------------------------------------------
