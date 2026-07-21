//-----------------------------------------------------------------------------
/*
 File        : GLabel.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GLabel - static text / image label inside a Dialog.
               Supports image+text, image-only, text-only and register-info
               (name / max / min / title) display modes.
               Per SG1210v25 dialog spec section 3.2.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec;
              ctor takes dialog origin so labels draw at absolute coords)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GLABEL_H
#define GUI_GLABEL_H

#include "GDialogDefs.h"
#include "GUI.h"
#include "GWidget.h"
#include <GUI_Type.h>
#include <cstdint>

class GDialog;

//=============================================================================
// class GLabel (spec 3.2)
//-----------------------------------------------------------------------------
class GLabel : public GWidget
{
public:
  struct GConfig
  {
    uint16_t         x, y, w, h;     // area: text/image; relative to Dialog
    uint16_t         regDraw;        // draw from register info (GDrawRegister)
    uint16_t         drawMode;       // GUI_DRAWMODE_NORMAL/GUI_DRAWMODE_TRANS
    uint16_t         alignment;      // GUI_TA_xxx combination
    uint32_t         crBackground,   // color of Label's background
                     crText;         // color of Text
    const GUI_FONT*  ftText;         // Text font
    GCSGAtlasImage   image;          // atlas + picture index
    uint16_t         uTextId;        // text Ident (multi-language)
    const char*      pText;          // text (priority over uTextId)
  };

  enum GDrawRegister
  {
    drNone   = 0,  //
    drName   = 1,  // Draw Register name
    drMax    = 2,  // Draw Register Max
    drMin    = 3,  // Draw Register Min
    drTitle  = 4   // Dialog Title
  };

public:
  // dlgX0/dlgY0: Dialog top-left (absolute), added to config coords
  GLabel(GDialog* pOwner, const GConfig* pConfig, int dlgX0, int dlgY0);

  virtual ~GLabel() = default;  // virtual destructor

  virtual void onShow();

protected:
  GRect           m_Rect = {0};             // absolute label rect
  const GConfig*  m_pConfig = nullptr;
};

//-----------------------------------------------------------------------------
#endif // GUI_GLABEL_H
