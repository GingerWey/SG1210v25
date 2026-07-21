//-----------------------------------------------------------------------------
/*
 File        : GDatetimeDialog.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GDatetimeDialog - date/time editor dialog (stub).
               Per SG1210v25 dialog spec section 8.

 Date        : 2026.07.21 (V1.00 - stub implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GDATETIMEDIALOG_H
#define GUI_GDATETIMEDIALOG_H

#include "DevTypes.h"
#include "GDialog.h"

//=============================================================================
// class GDatetimeDialog (spec 8.1)
//-----------------------------------------------------------------------------
class GDatetimeDialog : public GDialog
{
public:
  GDatetimeDialog(const GDialogConfig* pConfig, const void* Param);

  virtual ~GDatetimeDialog() = default;

  TDateTimeType getResult();

  virtual const char* getTitle() const { return nullptr; }
};

//-----------------------------------------------------------------------------
#endif // GUI_GDATETIMEDIALOG_H
