//-----------------------------------------------------------------------------
/*
 File        : GNumRegDialog.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GNumRegDialog - numeric register editor dialog.
               Handles INT/HEX/REAL register types with range validation.
               Per SG1210v25 dialog spec section 5.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GNUMREGDIALOG_H
#define GUI_GNUMREGDIALOG_H

#include "DevTypes.h"
#include "GDialog.h"
#include <cstdint>

//=============================================================================
// class GNumRegDialog (spec 5.3)
//-----------------------------------------------------------------------------
class GNumRegDialog : public GDialog
{
public:
  GNumRegDialog(const GDialogConfig* pConfig, const void* Param);

  virtual ~GNumRegDialog() = default;

  void setRegNum(uint32_t uRegNum);

  uint32_t getResult() const;

  // get dialog title (returns register name)
  virtual const char* getTitle() const;

  // Accessor for GLabel
  virtual uint32_t getRegNum() const { return m_uRegNum; }

protected:
  uint32_t               m_uRegNum = 0;
  const TDevRegInfoItem* m_pProperty = nullptr;
};

//-----------------------------------------------------------------------------
#endif // GUI_GNUMREGDIALOG_H
