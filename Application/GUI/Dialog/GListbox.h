//-----------------------------------------------------------------------------
/*
 File        : GListbox.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GListbox - in-place listbox editor for SIT_VAT_ENUM registers.
               Per SG1210v25 dialog spec section 9.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GLISTBOX_H
#define GUI_GLISTBOX_H

#include "GWidget.h"
#include <GUIConf.h>
#include <GUI_Type.h>
#include <cstdint>

//=============================================================================
// class GListbox (spec 9.1)
//-----------------------------------------------------------------------------
class GListbox : public GWidget
{
public:
  struct GStyle
  {
    uint32_t        crBackground,     // color of Listbox's background
                    crFrame,          // color of Listbox's frame line
                    crSelected,       // color of Selected background
                    crText,           // color of Text
                    crTextFocus;      // color of Text when focused
    const GUI_FONT* ftText;           // Text font
  };

  struct GConfig
  {
    uint16_t         x, y, w, h;       // Listbox area (absolute desktop coords)
    uint8_t          margin;           // margin around list
    uint8_t          drawMode;         // GUI_DRAWMODE_NORMAL/TRANS
    uint8_t          alignment;        // GUI_TA_xxx combination

    uint8_t          ItemCount;
    const char* const* pStrings;       // string list (C-string array)
    const uint16_t*  pStrIds;          // Multi-Language items (priority: pStrings)
  };

public:
  GListbox(const GStyle* pStyle, const GConfig* pConfig, uint32_t curIndex);

  virtual ~GListbox() = default;

  virtual void onShow();

  // get edited result (raw index)
  virtual uint32_t getValue() const { return m_uCurItem; }

  // alias of getValue() to match modal dialog contract (E-Trace)
  uint32_t getResult() const { return m_uCurItem; }

  virtual const char* getString();

protected:
  virtual void onKeyDown(uint32_t uKey, uint32_t uRepeat = 0);

#if GUI_SUPPORT_TOUCH
  virtual void onTouchDown(int x, int y);
  virtual void onTouchMove(int dx, int dy) { (void)dx; (void)dy; }
  virtual void onTouchUp  (int dx, int dy) { (void)dx; (void)dy; }
#endif

  void accept() const;
  void cancel() const;

protected:
  const GStyle*  m_pStyle  = nullptr;
  const GConfig* m_pConfig = nullptr;

  uint32_t       m_uTopItem  = 0;
  uint32_t       m_uCurItem  = 0;
};

//-----------------------------------------------------------------------------
#endif // GUI_GLISTBOX_H
