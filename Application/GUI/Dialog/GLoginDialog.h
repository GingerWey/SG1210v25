//-----------------------------------------------------------------------------
/*
 File        : GLoginDialog.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GLoginDialog - password input dialog for authorization.
               Per SG1210v25 dialog spec section 6.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GLOGINDIALOG_H
#define GUI_GLOGINDIALOG_H

#include "GDialog.h"
#include <cstdint>

//=============================================================================
// class GLoginDialog (spec 6.1)
//-----------------------------------------------------------------------------
class GLoginDialog : public GDialog
{
public:
  enum GLoginMode {
    lmLoginNormal   = 0,     // Normal login
    lmLoginAdmin    = 1,     // Administration Login
    lmPassword      = 2,     // get password (change password etc)
    lmDynaPassword  = 3      // dynamic password
  };

public:
  GLoginDialog(const GDialogConfig* pConfig, const void* Param);

  virtual ~GLoginDialog() = default;

  virtual void Init();

  // get dialog title (returns title based on mode)
  virtual const char* getTitle() const;

  // E-11 CLARIFIED: GLoginDialog has no getResult() - writes password
  // to register in accept() for security; Form checks GetPassword1Ok flag

protected:
  virtual void accept();

protected:
  GLoginMode  m_mode = lmLoginNormal;
  uint32_t    m_uRegNum = 0;
};

//-----------------------------------------------------------------------------
#endif // GUI_GLOGINDIALOG_H
