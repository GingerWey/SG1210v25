//-----------------------------------------------------------------------------
/*
 File        : GWidget.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GWidget - base class of all Dialog framework GUI elements.
               Parses GM_MESSAGE and dispatches to virtual handlers.
               Per SG1210v25 dialog spec section 3.1.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GWIDGET_H
#define GUI_GWIDGET_H

#include "GUIConf.h"
#include "GUIMessage.h"
#include <cstdint>

//=============================================================================
// class GWidget (spec 3.1)
//-----------------------------------------------------------------------------
class GWidget
{
public:
  GWidget(GWidget* pParent)
  {
    m_pParent = pParent;
    // NOTE: Do NOT call the virtual Init() here - a virtual call in the
    //       base ctor dispatches to GWidget::Init(), never the override.
    //       The owner must call Init() explicitly after construction.
  }

  virtual ~GWidget() = default;  // virtual destructor

  // Two-phase init: called by the owner after the object is fully constructed
  virtual void Init() {}

  virtual void onShow() = 0;
  virtual void onClose() {}

  // set focus on or off
  virtual void setFocus(bool bFocused)
  {
    if( bFocused != m_bFocused ) {
      m_bFocused = bFocused;
      onShow();
    }
  }

  bool focused() const { return m_bFocused; }

  // message from Parent
  virtual void onMessage(GM_MESSAGE* pMsg);

  // Physical keys, from Form
  // (public: owner widgets route raw keys down the widget tree)
  virtual void onKeyDown(uint32_t uKey, uint32_t uRepeat = 0) { (void)uKey; (void)uRepeat; }
  virtual void onKeyUp  (uint32_t uKey) { (void)uKey; }

#if GUI_SUPPORT_TOUCH
  virtual void onTouchDown(int  x, int  y);
  virtual void onTouchMove(int dx, int dy) { (void)dx; (void)dy; }
  virtual void onTouchUp  (int dx, int dy) { (void)dx; (void)dy; m_bTouchActive = false; }
#endif

protected:
  // Timer tick
  virtual void onTick(uint32_t uTick) { (void)uTick; }

protected:
  bool     m_bFocused = false;
  GWidget* m_pParent  = nullptr;

#if GUI_SUPPORT_TOUCH
  bool     m_bTouchActive = false;
  int      m_iTouchStartX = 0,
           m_iTouchStartY = 0;
#endif
};

//-----------------------------------------------------------------------------
#endif // GUI_GWIDGET_H
