//-----------------------------------------------------------------------------
/*
 File        : GDialogDefs.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : Common basic types shared by the Dialog framework.
               Per SG1210v25 dialog spec section 2.2 (basic data structures).

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GDIALOGDEFS_H
#define GUI_GDIALOGDEFS_H

#include "GUIPicture.h"
#include <cstdint>

//=============================================================================
// GRect Structure (spec 2.2.1)
//-----------------------------------------------------------------------------
struct GRect {
  int x, y;     // Top-left corner position
  int w, h;     // Width and height
};

//=============================================================================
// GDialogType enum (spec 2.2.2)
//-----------------------------------------------------------------------------
enum GDialogType
{
  dlgNone      = 0,   // No dialog exists
  dlgInplace   = 5,   // inplace editor, eg.: GListbox
  dlgLogin     = 10,  // Login dialog is executing
  dlgEditor    = 20,  // Edit dialog is executing
  dlgComfirm   = 30,  // Confirm message box is shown
  dlgMessage   = 32   // Information message box is shown
};

//=============================================================================
// GDialogTodo enum (spec 2.2.3)
//-----------------------------------------------------------------------------
enum GDialogTodo
{
  dtdNone    = 0,     // nothing to do
  dtdEdit    = 1,     // start edit
  dtdSave    = 2,     // save all list data
  dtdCancel  = 3      // restore all list data
};

//=============================================================================
// GCSGAtlasImage Structure (spec 2.2.4)
//-----------------------------------------------------------------------------
struct GCSGAtlasImage
{
  const TGUIPicture*   pAtlas;     // CSG Atlas
  uint32_t             uIndex;     // picture index constants
};

//-----------------------------------------------------------------------------
#endif // GUI_GDIALOGDEFS_H
