//-----------------------------------------------------------------------------
/*
 File        : GListbox.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GListbox - in-place listbox editor implementation.
               Per SG1210v25 dialog spec section 9.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "DevDebug.h"
#include "GFormCentra.h"
#include "GListbox.h"
#include "GUI.h"
#include "GUIMessage.h"
#include "Strings/TextStrs.h"
#include <GUIConf.h>
#include <GUI_Type.h>

//-----------------------------------------------------------------------------
GListbox::GListbox(const GStyle* pStyle, const GConfig* pConfig, uint32_t curIndex)
  : GWidget(nullptr)
{
  m_pStyle   = pStyle;
  m_pConfig  = pConfig;
  m_uCurItem = curIndex;
}

//-----------------------------------------------------------------------------
const char* GListbox::getString()
{
  DEV_ASSERT( nullptr == m_pConfig, GFC_EmptyPtr );

  if( nullptr != m_pConfig && m_pConfig->ItemCount > m_uCurItem ) {
    if( nullptr != m_pConfig->pStrings ) {
      return m_pConfig->pStrings[m_uCurItem];
    } else if( nullptr != m_pConfig->pStrIds ) {
      return GetMultiLangString(m_pConfig->pStrIds[m_uCurItem]);
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void GListbox::onShow()
{
  DEV_ASSERT(( nullptr == m_pConfig || nullptr == m_pStyle ), GFC_EmptyPtr);

  int x0 = m_pConfig->x;
  int y0 = m_pConfig->y;
  int x1 = m_pConfig->x + m_pConfig->w - 1;
  int y1 = m_pConfig->y + m_pConfig->h - 1;

  // Background
  GUI_SetColor(m_pStyle->crBackground);
  GUI_FillRect(x0, y0, x1, y1);

  // Frame
  GUI_SetColor(m_pStyle->crFrame);
  GUI_DrawRect(x0, y0, x1, y1);

  // Draw items (simplified - draw all visible items)
  if( nullptr != m_pStyle->ftText ) {
    GUI_SetFont(m_pStyle->ftText);
  }
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);

  int iRowH = 20;  // simplified row height
  int iContentX0 = x0 + m_pConfig->margin;
  int iContentX1 = x1 - m_pConfig->margin;

  for( uint32_t i = 0; i < m_pConfig->ItemCount; i++ ) {
    int iY0 = y0 + m_pConfig->margin + i * iRowH;
    int iY1 = iY0 + iRowH - 1;

    if( iY1 > y1 - m_pConfig->margin ) {
      break;  // out of visible area
    }

    // Highlight current item
    if( i == m_uCurItem ) {
      GUI_SetColor(m_pStyle->crSelected);
      GUI_FillRect(iContentX0, iY0, iContentX1, iY1);
    }

    const char* pStr = nullptr;
    if( nullptr != m_pConfig->pStrings ) {
      pStr = m_pConfig->pStrings[i];
    } else if( nullptr != m_pConfig->pStrIds ) {
      pStr = GetMultiLangString(m_pConfig->pStrIds[i]);
    }

    if( nullptr != pStr ) {
      GUI_SetColor((i == m_uCurItem) ? m_pStyle->crTextFocus : m_pStyle->crText);
      GUI_RECT r;
      r.x0 = iContentX0;
      r.y0 = iY0;
      r.x1 = iContentX1;
      r.y1 = iY1;
      GUI_DispStringInRect(pStr, &r, m_pConfig->alignment);
    }
  }
}

//-----------------------------------------------------------------------------
void GListbox::onKeyDown(uint32_t uKey, uint32_t uRepeat)
{
  (void)uRepeat;

  switch( uKey ) {
    case KEY_ENTER:
      accept();
      break;
    case KEY_ESCAPE:
      cancel();
      break;
    case KEY_UP:
      // focus previous item (clamp at top)
      if( 0 < m_uCurItem ) {
        m_uCurItem--;
        onShow();
      }
      break;
    case KEY_DOWN:
      // focus next item (clamp at bottom)
      if( nullptr != m_pConfig && m_pConfig->ItemCount - 1 > m_uCurItem ) {
        m_uCurItem++;
        onShow();
      }
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
#if GUI_SUPPORT_TOUCH
void GListbox::onTouchDown(int x, int y)
{
  GWidget::onTouchDown(x, y);

  if( nullptr == m_pConfig ) {
    return;
  }

  // Check if touch is inside listbox
  if( m_pConfig->x <= x && x < m_pConfig->x + m_pConfig->w &&
      m_pConfig->y <= y && y < m_pConfig->y + m_pConfig->h ) {
    // Hit test items (simplified)
    int iRowH = 20;
    int iRelY = y - (m_pConfig->y + m_pConfig->margin);
    if( 0 <= iRelY ) {
      uint32_t idx = iRelY / iRowH;
      if( idx < m_pConfig->ItemCount ) {
        m_uCurItem = idx;
        accept();
      }
    }
  } else {
    // Touch outside -> cancel
    cancel();
  }
}
#endif

//-----------------------------------------------------------------------------
void GListbox::accept() const
{
  gfc::PostMsg(GM_DIALOG_ACCEPT, static_cast<uint16_t>(m_uCurItem), 0);
}

//-----------------------------------------------------------------------------
void GListbox::cancel() const
{
  gfc::PostMsg(GM_DIALOG_CANCEL, 0, 0);
}
//-----------------------------------------------------------------------------
