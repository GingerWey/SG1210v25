//-----------------------------------------------------------------------------
/*
 File        : GDialog.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GDialog - modal dialog base class implementation.
               Per SG1210v25 dialog spec section 3.8.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GDialog.h"
#include "GEditorPanel.h"

#include "DevDebug.h"
#include "GFormCentra.h"
#include "GUI.h"
#include "GUIMessage.h"
#include "Graphics/ImageRes.h"
#include "RamHeap.h"

#include "GKeyboard.h"
#include "GLabel.h"
#include "GWidget.h"
#include <GUIConf.h>
#include <cstdint>
#include <new>

//=============================================================================
// GDialog
//-----------------------------------------------------------------------------
GDialog::GDialog(const GDialogConfig* pConfig, const void* Param)
  : GWidget(nullptr)   // Dialog is top-level, no parent widget
{
  m_pConfig = pConfig;
  (void)Param;  // derived class extracts Param
}

//=============================================================================
// GDialog::Init - two-phase initialization (E-19, E-20)
//-----------------------------------------------------------------------------
void GDialog::Init()
{
  DEV_ASSERT( nullptr == m_pConfig, GFC_EmptyPtr );

  // 1. Create Labels (if defined in config)
  if( 0 < m_pConfig->labelCount && nullptr != m_pConfig->pLabels ) {
    uint32_t n = m_pConfig->labelCount;
    uint8_t* ptr = (uint8_t*)RAM_Malloc(n * sizeof(GLabel));
    DEV_ASSERT( nullptr == ptr, GFC_OutOfMem );
    if( nullptr != ptr ) {
      m_pLabels = (GLabel*)ptr;
      // Assume dialog at (15, 5) per spec layout
      int dlgX0 = 15, dlgY0 = 5;
      for( uint32_t i = 0; i < n; i++ ) {
        new (ptr) GLabel(this, m_pConfig->pLabels + i, dlgX0, dlgY0);
        ptr += sizeof(GLabel);
      }
    }
  }

  // 2. Create Keyboard (if defined in config)
  if( nullptr != m_pConfig->pKbdStyle && nullptr != m_pConfig->pKeyGrid ) {
    void* ptr = RAM_Malloc(sizeof(GKeyboard));
    DEV_ASSERT( nullptr == ptr, GFC_OutOfMem );
    if( nullptr != ptr ) {
      m_pKeyboard = new (ptr) GKeyboard(this, m_pConfig->pKbdStyle,
                                         m_pConfig->pKeyGrid);
      // CRITICAL (E-22): Set keyboard rect BEFORE Init() so GKey constructors
      // have valid kbdX0/kbdY0
      m_pKeyboard->setRect(m_pConfig->kbdX, m_pConfig->kbdY,
                           m_pConfig->kbdW, m_pConfig->kbdH);
      m_pKeyboard->Init();
    }
  }

  // 3. Create EditorPanel (if defined in config) - forward declaration used
  if( nullptr != m_pConfig->pEditorStyle && nullptr != m_pConfig->pEditorPanel ) {
    void* ptr = RAM_Malloc(sizeof(GEditorPanel));
    DEV_ASSERT( nullptr == ptr, GFC_OutOfMem );
    if( nullptr != ptr ) {
      m_pEditors = new (ptr) GEditorPanel(this,
                                           (const GEditorPanel::GStyle*)m_pConfig->pEditorStyle,
                                           (const GEditorPanel::GConfig*)m_pConfig->pEditorPanel);
      m_pEditors->Init();
    }
  }
}

//=============================================================================
// GDialog::onShow
//-----------------------------------------------------------------------------
void GDialog::onShow()
{
  drawBackground();
  drawDialog();
  drawLabels();
  drawEditorPanel();
  drawKeyboard();
}

//-----------------------------------------------------------------------------
void GDialog::onClose()
{
  // Cleanup before teardown (derived classes override for custom cleanup)
}

//=============================================================================
// GDialog::onKeyDown - physical keys from Form (spec 3.8.3.6)
//-----------------------------------------------------------------------------
void GDialog::onKeyDown(uint32_t uKey, uint32_t uRepeat)
{
  // E-6 CLARIFIED: KEY_ESCAPE direct to cancel, others to keyboard
  if( KEY_ESCAPE == uKey ) {
    cancel();
    return;
  }

  if( nullptr != m_pKeyboard ) {
    m_pKeyboard->onKeyDown(uKey, uRepeat);
  }
}

//=============================================================================
// GDialog::onGKeyPress - semantic key from GKey (spec 3.8.3.7)
//-----------------------------------------------------------------------------
void GDialog::onGKeyPress(uint32_t uKey, uint32_t uRepeat)
{
  switch( uKey ) {
    case KEY_ENTER:
      accept();
      break;
    case KEY_ESCAPE:
      cancel();
      break;
    default:
      if( nullptr != m_pEditors ) {
        m_pEditors->onGKeyPress(uKey, uRepeat);
      }
      break;
  }
}

//=============================================================================
// GDialog::accept - validate editors and post GM_DIALOG_ACCEPT (E-18 fixed)
//-----------------------------------------------------------------------------
void GDialog::accept()
{
  // Validate editors if present (message-type dialogs have no editors)
  if( nullptr != m_pEditors ) {
    if( false == m_pEditors->validate() ) {
      // Validation failed - show error prompt and keep dialog open
      // TODO: Show validation error message
      return;
    }
  }

  gfc::PostMsgPtr(GM_DIALOG_ACCEPT, 0, this);
}

//-----------------------------------------------------------------------------
void GDialog::cancel()
{
  gfc::PostMsgPtr(GM_DIALOG_CANCEL, 0, this);
}

//=============================================================================
// Drawing methods
//-----------------------------------------------------------------------------
void GDialog::drawBackground()
{
  // Draw desktop background (overridden by derived class if needed)
  GUI_SetBkColor(0x000000);
  GUI_Clear();
}

//-----------------------------------------------------------------------------
void GDialog::drawDialog()
{
  // Draw dialog frame at (15,5,290,230) per spec layout
  constexpr int DLG_X0 = 15, DLG_Y0 = 5, DLG_W = 290, DLG_H = 230;
  int x1 = DLG_X0 + DLG_W - 1;
  int y1 = DLG_Y0 + DLG_H - 1;

  // Fill dialog background
  GUI_SetColor(0x031635);
  GUI_FillRoundedRect(DLG_X0, DLG_Y0, x1, y1, 3);

  // Outer frame
  GUI_SetColor(0x2F8FD0);
  GUI_DrawRoundedRect(DLG_X0, DLG_Y0, x1, y1, 3);

  // Inner frame (inset 4px)
  GUI_SetColor(0x0A2240);
  GUI_DrawRoundedRect(DLG_X0 + 4, DLG_Y0 + 4, x1 - 4, y1 - 4, 3);
}

//-----------------------------------------------------------------------------
void GDialog::drawLabels()
{
  if( nullptr != m_pLabels && nullptr != m_pConfig ) {
    for( uint32_t i = 0; i < m_pConfig->labelCount; i++ ) {
      m_pLabels[i].onShow();
    }
  }
}

//-----------------------------------------------------------------------------
void GDialog::drawKeyboard()
{
  if( nullptr != m_pKeyboard ) {
    m_pKeyboard->onShow();
  }
}

//-----------------------------------------------------------------------------
void GDialog::drawEditorPanel()
{
  if( nullptr != m_pEditors ) {
    m_pEditors->onShow();
  }
}
//-----------------------------------------------------------------------------
