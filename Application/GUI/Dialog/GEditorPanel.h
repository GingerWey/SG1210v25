//-----------------------------------------------------------------------------
/*
 File        : GEditorPanel.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GEditor + GNumberEditor + GEditorPanel classes.
               GEditor is the base editor widget; GNumberEditor handles
               numeric input with range/format; GEditorPanel manages an
               array of editors with focus switching.
               Per SG1210v25 dialog spec sections 3.5 / 3.6 / 3.7.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GEDITORPANEL_H
#define GUI_GEDITORPANEL_H

#include "GDialogDefs.h"
#include "GWidget.h"
#include <GUI_Type.h>
#include <cstdint>

class GDialog;

//=============================================================================
// class GEditor (spec 3.5)
//-----------------------------------------------------------------------------
class GEditor : public GWidget
{
public:
  struct GStyle
  {
    uint32_t        crBackground,     // color of Editor's background
                    crFrame,          // color of Editor's frame line
                    crFrameFocus,     // color of Editor's frame line when focus
                    crCursor,         // color of Cursor
                    crText,           // color of Text
                    crTextFocus;      // color of Text when focused
    const GUI_FONT* ftText;           // Text font
  };

  struct GConfig
  {
    uint16_t  xEdt, yEdt, wEdt, hEdt;  // editor area: relative to GEditorPanel
    uint16_t  xTxt, yTxt, wTxt, hTxt;  // text area: relative to editor
    uint16_t  drawMode;                // GUI_DRAWMODE_NORMAL/TRANS
    uint16_t  alignment;               // GUI_TA_xxx combination
  };

  enum GKeyStatus
  {
    krAccepted      = 0,  // Key accepted
    krRejected      = 1,  // Key rejected (overflow, invalid)
    krMovePrev      = 2,  // Move to previous editor
    krMoveNext      = 3,  // Move to next editor
    krMoveUp        = 4,  // Move to upper editor (multi-line)
    krMoveDown      = 5   // Move to lower editor (multi-line)
  };

public:
  GEditor(GDialog* pOwner, const GStyle* pStyle,
          const GConfig* pConfig, int x0, int y0);

  virtual ~GEditor();

  virtual void Init();

  // Validate current editor value; base returns true (no constraint)
  virtual bool validate() const { return true; }

  virtual void onShow();

  // set value display format
  virtual void setFormat(const char* pfmt)
  {
    m_pFormat = pfmt;
  }

  // Key message from keyboard
  virtual GKeyStatus onGKeyPress(uint32_t uKey, uint32_t uRepeat = 0);

  // get/set value (overridden by derived classes)
  virtual int  getValue() const { return 0; }
  virtual void setValue(int value) { (void)value; }

protected:
  // Timer tick - flash cursor
  virtual void onTick(uint32_t uTick);

protected:
  GRect          m_Rect;
  const GStyle*  m_pStyle  = nullptr;
  const GConfig* m_pConfig = nullptr;
  const char*    m_pFormat = nullptr;

  char*          m_pText   = nullptr;   // cache for edit
  int            m_CursorPos = 0;
};

//=============================================================================
// class GNumberEditor (spec 3.6)
//-----------------------------------------------------------------------------
class GNumberEditor : public GEditor
{
public:
  GNumberEditor(GDialog* pOwner, const GStyle* pStyle,
                const GConfig* pConfig, int epX0, int epY0);

  virtual ~GNumberEditor() = default;  // virtual destructor

  virtual void Init();

  virtual void onShow();

  // Set Range
  void setRange(const int vMin, const int vMax);

  // get edited result
  virtual void setValue(int value);
  virtual int  getValue() const;
  virtual bool validate() const;

  // from GKey
  virtual GKeyStatus onGKeyPress(uint32_t uKey, uint32_t uRepeat = 0);

protected:
  int     m_vMin = 0, m_vMax = 0;  // Range
  int     m_Value = 0;
};

//=============================================================================
// class GEditorPanel (spec 3.7)
//-----------------------------------------------------------------------------
class GEditorPanel : public GWidget
{
public:
  struct GStyle
  {
    uint32_t       crBackground, // color of Editor's background
                   crFrame;      // color of Editor's frame line
    GEditor::GStyle editorStyle;  // style of editor
  };

  struct GConfig
  {
    uint16_t  x, y, w, h;         // Panel area, offset by Dialog Left-top
    uint32_t  Count;
    const GEditor::GConfig* Editors;
  };

public:
  GEditorPanel(GDialog* pDialog, const GStyle* pStyle, const GConfig* pConfig,
               int iCurIndex = 0);

  virtual ~GEditorPanel();

  virtual void Init();
  virtual void onShow();

  // get edited result
  virtual GEditor* operator[] (uint32_t index)
  {
    if( index < m_pConfig->Count && nullptr != m_Editors ) {
      return m_Editors + index;
    }
    return nullptr;
  }

  virtual bool validate() const;

  // from GKey (E-9: single entry point, receives semantic key events)
  virtual void onGKeyPress(uint32_t uKey, uint32_t uRepeat = 0);

protected:
  const GStyle*  m_pStyle   = nullptr;
  const GConfig* m_pConfig  = nullptr;

  int            m_CurIndex = 0;  // Focused editor index
  GEditor*       m_Editors  = nullptr;
};

//-----------------------------------------------------------------------------
#endif // GUI_GEDITORPANEL_H
