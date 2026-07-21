//-----------------------------------------------------------------------------
/*
 File        : GIPAddressDialog.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GIPAddressDialog - IPv4 address editor dialog (stub).
               Per SG1210v25 dialog spec section 7.

 Date        : 2026.07.21 (V1.00 - stub implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GIPADDRESSDIALOG_H
#define GUI_GIPADDRESSDIALOG_H

#include "GDialog.h"
#include <cstdint>

//=============================================================================
// class GIPAddressDialog (spec 7.1)
//-----------------------------------------------------------------------------
class GIPAddressDialog : public GDialog
{
public:
  GIPAddressDialog(const GDialogConfig* pConfig, const void* Param);

  virtual ~GIPAddressDialog() = default;

  void     setRegNum(uint32_t uRegNum);
  uint32_t getResult() const;

  virtual const char* getTitle() const;
  virtual uint32_t getRegNum() const { return m_uRegNum; }

protected:
  uint32_t   m_uRegNum = 0;
};

//-----------------------------------------------------------------------------
#endif // GUI_GIPADDRESSDIALOG_H
