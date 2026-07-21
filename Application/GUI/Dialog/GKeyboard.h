//-----------------------------------------------------------------------------
/*
 File        : GKeyboard.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GKey + GKeyboard - virtual keyboard of the Dialog framework.
               GKey is a single key widget; GKeyboard manages a key grid,
               moves key focus with arrow keys and dispatches semantic key
               presses to the owner Dialog.
               Per SG1210v25 dialog spec sections 3.3 / 3.4.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec;
              GKeyGrid uses a flat key array of rowCount*colCount entries)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GKEYBOARD_H
#define GUI_GKEYBOARD_H

#include "GWidget.h"
#include <GUIConf.h>
#include "GDialogDefs.h"
#include <GUI_Type.h>
#include <cstdint>

class GDialog;

//=============================================================================
// class GKey (spec 3.3)
//-----------------------------------------------------------------------------
class GKey : public GWidget
{
public:
  struct GStyle
  {
    uint32_t         crBackground,    // color of key's background
                     crFrame,         // color of key's frame line (normal)
                     crFrameFocus,    // color of key's frame line (focused)
                     crLabel,         // color of key's label text (normal)
                     crLabelFocus;    // color of key's label text (focused)
    const GUI_FONT*  ftLabel;         // Label font
  };

  struct GConfig
  {
    uint8_t          uInvalid;                // invalid key, placeholder only
    uint8_t          uKeyCode;                // key code
    uint16_t         xKey, yKey, wKey, hKey;  // key area: relative to Keyboard
    uint16_t         xLbl, yLbl, wLbl, hLbl;  // label area: relative to key
    uint8_t          drawMode;                // GUI_DRAWMODE_NORMAL/TRANS
    uint8_t          alignment;               // GUI_TA_xxx combination
    GCSGAtlasImage   image;                   // picture index
    uint16_t         uTextId;                 // multi-language string ID
    const char*      pText;                   // key name string (priority)
  };

public:
  GKey(GDialog* pDialog, const GStyle* pStyle, const GConfig* pConfig,
       const int kbdX0, const int kbdY0);

  virtual ~GKey() = default;  // virtual destructor

  virtual void onShow();

  uint8_t keyCode() const;
  bool    valid() const;

#if GUI_SUPPORT_TOUCH
  bool hitTest(int x, int y) const;
#endif

protected:
  GRect          m_Rect; // absolute key rect
  const GStyle*  m_pStyle  = nullptr;
  const GConfig* m_pConfig = nullptr;
};

//=============================================================================
// class GKeyboard (spec 3.4)
//-----------------------------------------------------------------------------
class GKeyboard : public GWidget
{
public:
  struct GStyle
  {
    uint32_t     crKbdBackground, // color of Keyboard's background
                 crKbdFrame;      // color of Keyboard's frame line
    GKey::GStyle keyStyle;        // style of key
  };

  struct GKeyGrid
  {
    uint8_t               rowCount,
                          colCount;
    const GKey::GConfig*  pKeys;  // flat array, rowCount*colCount entries
  };

public:
  GKeyboard(GDialog* pDialog, const GStyle* pStyle, const GKeyGrid* pKeyGrid,
            int iCurIndex = 0);

  virtual ~GKeyboard();  // frees the GKey array

  virtual void Init();

  virtual void onShow();

  // absolute keyboard rect; MUST be set before Init() (E-22)
  void setRect(int x, int y, int w, int h)
  {
    m_Rect.x = x;
    m_Rect.y = y;
    m_Rect.w = w;
    m_Rect.h = h;
  }

  // Physical keys, routed from GDialog::onKeyDown
  virtual void onKeyDown(uint32_t uKey, uint32_t uRepeat = 0);

#if GUI_SUPPORT_TOUCH
  virtual void onTouchDown(int x, int y);
#endif

protected:
  int keyCount() const;
  void moveFocus(int iNewIndex);

protected:
  GRect            m_Rect;
  const GStyle*    m_pStyle   = nullptr;
  const GKeyGrid*  m_pKeyGrid = nullptr;

  int              m_CurIndex = 0;        // Focused key index
  GKey*            m_pKeys    = nullptr;
};

//-----------------------------------------------------------------------------
#endif // GUI_GKEYBOARD_H
