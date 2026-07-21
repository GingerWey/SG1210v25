//-----------------------------------------------------------------------------
/*
 File        : GDialog.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GDialog - base class for modal dialogs in the framework.
               Manages keyboard + editor panel + labels lifecycle, routes
               messages from Form, handles accept/cancel completion flow.
               Per SG1210v25 dialog spec section 3.8.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec;
              includes E-19/E-20 two-phase Init pattern and keyboard coords)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GDIALOG_H
#define GUI_GDIALOG_H

#include "GKeyboard.h"
#include "GLabel.h"
#include "GWidget.h"
#include <cstdint>

class GEditorPanel;

//=============================================================================
// class GDialog (spec 3.8)
//-----------------------------------------------------------------------------
class GDialog : public GWidget
{
public:
  struct GDialogConfig
  {
    // labels
    uint32_t                     labelCount;
    const GLabel::GConfig*       pLabels;
    // Keyboard (coordinates are Dialog-relative, passed to setRect before Init)
    int                          kbdX, kbdY, kbdW, kbdH;  // Keyboard rect
    const GKeyboard::GStyle*     pKbdStyle;
    const GKeyboard::GKeyGrid*   pKeyGrid;
    // edit panel
    const void*                  pEditorStyle;   // GEditorPanel::GStyle*
    const void*                  pEditorPanel;   // GEditorPanel::GConfig*
  };

public:
  GDialog(const GDialogConfig* pConfig, const void* Param);

  virtual ~GDialog() = default;  // virtual destructor

  // Two-phase initialization: creates and initializes all child components
  virtual void Init();

  // calls from GUICentra / Form
  virtual void onShow();
  virtual void onClose();

  // Physical keys, from Form (E-6: KEY_ESCAPE direct, else to keyboard)
  virtual void onKeyDown(uint32_t uKey, uint32_t uRepeat = 0);

  // Semantic key from GKey (E-6: routes to editor panel)
  virtual void onGKeyPress(uint32_t uKey, uint32_t uRepeat = 0);

  // Accessors for GLabel register-info display
  virtual uint32_t    getRegNum() const { return 0; }
  virtual const char* getTitle() const = 0;

  // Public for GKeyboard to call
  virtual void cancel();

protected:
  // draw desktop background
  virtual void drawBackground();

  // draw Dialog background、frames
  virtual void drawDialog();

  // draw Labels
  virtual void drawLabels();

  // notify Keyboard to draw
  virtual void drawKeyboard();

  // notify editorpanel to draw
  virtual void drawEditorPanel();

  virtual void accept();

protected:
  const GDialogConfig* m_pConfig = nullptr;

  GKeyboard*    m_pKeyboard = nullptr;
  GEditorPanel* m_pEditors  = nullptr;
  GLabel*       m_pLabels   = nullptr;
};

//-----------------------------------------------------------------------------
#endif // GUI_GDIALOG_H
