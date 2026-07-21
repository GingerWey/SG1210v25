//-----------------------------------------------------------------------------
/*
 File        : GKeyboard.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GKey + GKeyboard implementation.
               Per SG1210v25 dialog spec sections 3.3 / 3.4.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GKeyboard.h"
#include "GDialog.h"

#include "GUI.h"
#include "CSGDraw.h"
#include "DevDebug.h"
#include "RamHeap.h"
#include "Strings/TextStrs.h"

#include <new>

//=============================================================================
// GKey
//-----------------------------------------------------------------------------
GKey::GKey(GDialog* pDialog, const GStyle* pStyle, const GConfig* pConfig,
           const int kbdX0, const int kbdY0)
  : GWidget(reinterpret_cast<GWidget*>(pDialog))
{
  m_pStyle  = pStyle;
  m_pConfig = pConfig;
  if( nullptr != pConfig ) {
    m_Rect.x = pConfig->xKey + kbdX0;
    m_Rect.y = pConfig->yKey + kbdY0;
    m_Rect.w = pConfig->wKey;
    m_Rect.h = pConfig->hKey;
  }
}

//-----------------------------------------------------------------------------
uint8_t GKey::keyCode() const
{
  if( nullptr != m_pConfig ) {
    return m_pConfig->uKeyCode;
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool GKey::valid() const
{
  if( nullptr != m_pConfig ) {
    return (0 == m_pConfig->uInvalid);
  }
  return false;
}

//-----------------------------------------------------------------------------
void GKey::onShow()
{
  DEV_ASSERT(( nullptr == m_pConfig || nullptr == m_pStyle ), GFC_EmptyPtr);

  if( 0 != m_pConfig->uInvalid ) {
    return;   // placeholder key, nothing to draw
  }

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

  // Label: image (left) + text
  int iImgW = 0;
  if( nullptr != m_pConfig->image.pAtlas ) {
    CSG_DrawPicture(m_pConfig->image.pAtlas,
                    m_Rect.x + m_pConfig->xLbl,
                    m_Rect.y + m_pConfig->yLbl,
                    m_pConfig->image.uIndex, 100, nullptr);
    iImgW = m_pConfig->hLbl;
  }

  const char* pStr = nullptr;
  if( nullptr != m_pConfig->pText ) {
    pStr = m_pConfig->pText;
  } else if( 0 != m_pConfig->uTextId ) {
    pStr = GetMultiLangString(m_pConfig->uTextId);
  }

  if( nullptr != pStr ) {
    if( nullptr != m_pStyle->ftLabel ) {
      GUI_SetFont(m_pStyle->ftLabel);
    }
    GUI_SetColor((true == m_bFocused) ? m_pStyle->crLabelFocus : m_pStyle->crLabel);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);

    GUI_RECT rLbl;
    rLbl.x0 = m_Rect.x + m_pConfig->xLbl + iImgW;
    rLbl.y0 = m_Rect.y + m_pConfig->yLbl;
    rLbl.x1 = m_Rect.x + m_pConfig->xLbl + m_pConfig->wLbl - 1;
    rLbl.y1 = m_Rect.y + m_pConfig->yLbl + m_pConfig->hLbl - 1;
    GUI_DispStringInRect(pStr, &rLbl, m_pConfig->alignment);
  }
}

//-----------------------------------------------------------------------------
#if GUI_SUPPORT_TOUCH
bool GKey::hitTest(int x, int y) const
{
  if( nullptr == m_pConfig || 0 != m_pConfig->uInvalid ) {
    return false;
  }
  return (m_Rect.x <= x && x < m_Rect.x + m_Rect.w &&
          m_Rect.y <= y && y < m_Rect.y + m_Rect.h);
}
#endif

//=============================================================================
// GKeyboard
//-----------------------------------------------------------------------------
GKeyboard::GKeyboard(GDialog* pDialog, const GStyle* pStyle,
                     const GKeyGrid* pKeyGrid, int iCurIndex)
  : GWidget(reinterpret_cast<GWidget*>(pDialog))
{
  m_pStyle   = pStyle;
  m_pKeyGrid = pKeyGrid;
  m_CurIndex = iCurIndex;
}

//-----------------------------------------------------------------------------
GKeyboard::~GKeyboard()
{
  if( nullptr != m_pKeys ) {
    int n = keyCount();
    for( int i = 0; i < n; i++ ) {
      m_pKeys[i].~GKey();
    }
    RAM_Free(m_pKeys);
    m_pKeys = nullptr;
  }
}

//-----------------------------------------------------------------------------
int GKeyboard::keyCount() const
{
  if( nullptr != m_pKeyGrid ) {
    return m_pKeyGrid->rowCount * m_pKeyGrid->colCount;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void GKeyboard::Init()
{
  DEV_ASSERT(( nullptr == m_pKeyGrid || nullptr == m_pStyle ), GFC_EmptyPtr);

  int n = keyCount();
  if( 0 >= n ) {
    return;
  }

  uint8_t* ptr = (uint8_t*)RAM_Malloc(n * sizeof(GKey));
  DEV_ASSERT( nullptr == ptr, GFC_OutOfMem );
  m_pKeys = (GKey*)ptr;

  GDialog* pDlg = reinterpret_cast<GDialog*>(m_pParent);
  for( int i = 0; i < n; i++ ) {
    new (ptr) GKey(pDlg, &(m_pStyle->keyStyle),
                   m_pKeyGrid->pKeys + i,
                   m_Rect.x, m_Rect.y);
    ptr += sizeof(GKey);
  }

  // Initial focus on first valid key
  int iFirst = -1;
  for( int i = 0; i < n; i++ ) {
    if( true == m_pKeys[i].valid() ) {
      iFirst = i;
      break;
    }
  }
  if( 0 <= iFirst ) {
    m_CurIndex = iFirst;
    m_pKeys[m_CurIndex].setFocus(true);
  }
}

//-----------------------------------------------------------------------------
void GKeyboard::onShow()
{
  DEV_ASSERT( nullptr == m_pStyle, GFC_EmptyPtr );

  int x0 = m_Rect.x;
  int y0 = m_Rect.y;
  int x1 = m_Rect.x + m_Rect.w - 1;
  int y1 = m_Rect.y + m_Rect.h - 1;

  GUI_SetColor(m_pStyle->crKbdBackground);
  GUI_FillRect(x0, y0, x1, y1);
  GUI_SetColor(m_pStyle->crKbdFrame);
  GUI_DrawRect(x0, y0, x1, y1);

  int n = keyCount();
  for( int i = 0; i < n; i++ ) {
    m_pKeys[i].onShow();
  }
}

//-----------------------------------------------------------------------------
void GKeyboard::moveFocus(int iNewIndex)
{
  int n = keyCount();
  if( 0 > iNewIndex || iNewIndex >= n ) {
    return;
  }
  if( false == m_pKeys[iNewIndex].valid() ) {
    return;
  }
  m_pKeys[m_CurIndex].setFocus(false);
  m_CurIndex = iNewIndex;
  m_pKeys[m_CurIndex].setFocus(true);
}

//=============================================================================
// GKeyboard::onKeyDown - physical keys routed from GDialog (spec 3.4, E-21)
//-----------------------------------------------------------------------------
void GKeyboard::onKeyDown(uint32_t uKey, uint32_t uRepeat)
{
  GDialog* pDialog = reinterpret_cast<GDialog*>(m_pParent);
  DEV_ASSERT(( nullptr == pDialog || nullptr == m_pKeys || nullptr == m_pKeyGrid ), GFC_EmptyPtr);

  int iCols = m_pKeyGrid->colCount;
  int iRows = m_pKeyGrid->rowCount;

  switch( uKey ) {
    case KEY_ENTER: {
      // Send focused key's keyCode as semantic event
      if( m_CurIndex < iRows * iCols ) {
        uint8_t keyCode = m_pKeys[m_CurIndex].keyCode();
        pDialog->onGKeyPress(keyCode, uRepeat);
      }
      break;
    }
    case KEY_ESCAPE:
      pDialog->cancel();
      break;
    case KEY_LEFT: {
      // Move focus to nearest valid key at left in current row
      int iRow = m_CurIndex / iCols;
      for( int idx = m_CurIndex - 1; idx >= iRow * iCols; idx-- ) {
        if( true == m_pKeys[idx].valid() ) {
          moveFocus(idx);
          break;
        }
      }
      break;
    }
    case KEY_RIGHT: {
      // Move focus to nearest valid key at right in current row
      int iRow = m_CurIndex / iCols;
      for( int idx = m_CurIndex + 1; idx < (iRow + 1) * iCols; idx++ ) {
        if( true == m_pKeys[idx].valid() ) {
          moveFocus(idx);
          break;
        }
      }
      break;
    }
    case KEY_UP: {
      // Move focus to upper row, same column
      int iCol = m_CurIndex % iCols;
      for( int row = (m_CurIndex / iCols) - 1; row >= 0; row-- ) {
        int idx = row * iCols + iCol;
        if( true == m_pKeys[idx].valid() ) {
          moveFocus(idx);
          break;
        }
      }
      break;
    }
    case KEY_DOWN: {
      // Move focus to lower row, same column
      int iCol = m_CurIndex % iCols;
      for( int row = (m_CurIndex / iCols) + 1; row < iRows; row++ ) {
        int idx = row * iCols + iCol;
        if( true == m_pKeys[idx].valid() ) {
          moveFocus(idx);
          break;
        }
      }
      break;
    }
    default:
      break;
  }
}

//=============================================================================
#if GUI_SUPPORT_TOUCH
//-----------------------------------------------------------------------------
// Touch on a key: focus it and send its keyCode to the Dialog
void GKeyboard::onTouchDown(int x, int y)
{
  GWidget::onTouchDown(x, y);

  GDialog* pDialog = reinterpret_cast<GDialog*>(m_pParent);
  if( nullptr == pDialog || nullptr == m_pKeys ) {
    return;
  }

  int n = keyCount();
  for( int i = 0; i < n; i++ ) {
    if( true == m_pKeys[i].hitTest(x, y) ) {
      moveFocus(i);
      pDialog->onGKeyPress(m_pKeys[i].keyCode(), 0);
      break;
    }
  }
}
#endif
//-----------------------------------------------------------------------------
