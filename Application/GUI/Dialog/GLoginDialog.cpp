//-----------------------------------------------------------------------------
/*
 File        : GLoginDialog.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GLoginDialog - password input dialog implementation.
               Per SG1210v25 dialog spec section 6.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GEditorPanel.h"
#include "GLoginDialog.h"

#include "DevRegs.h"
#include "GDialog.h"
#include "Strings/TextStrs.h"
#include <cstdint>

//-----------------------------------------------------------------------------
GLoginDialog::GLoginDialog(const GDialogConfig* pConfig, const void* Param)
  : GDialog(pConfig, Param)
{
  m_mode = *reinterpret_cast<const GLoginMode*>(Param);
}

//-----------------------------------------------------------------------------
void GLoginDialog::Init()
{
  switch( m_mode ) {
    case lmLoginNormal:
      m_uRegNum = REG_PASSWORD;
      break;
    case lmLoginAdmin:
      m_uRegNum = REG_PASSWORD2;
      break;
    case lmPassword:
      m_uRegNum = REG_FN_PASSWORD;
      break;
    case lmDynaPassword:
      m_uRegNum = REG_PASSWORD;
      break;
    default:
      m_uRegNum = 0;
      break;
  }

  GDialog::Init();
}

//-----------------------------------------------------------------------------
const char* GLoginDialog::getTitle() const
{
  switch( m_mode ) {
    case lmLoginNormal:
      return GetMultiLangString(idLoginCap1);
    case lmLoginAdmin:
      return GetMultiLangString(idLoginCap2);
    case lmPassword:
      return GetMultiLangString(idSetPSWDCap1);
    case lmDynaPassword:
      return GetMultiLangString(idLoginApply);
    default:
      return nullptr;
  }
}

//-----------------------------------------------------------------------------
void GLoginDialog::accept()
{
  // Validate password input (4-digit range check)
  if( nullptr != m_pEditors ) {
    if( false == m_pEditors->validate() ) {
      // Validation failed - invalid password format
      return;
    }
  }

  // Get password value
  uint32_t value = 0;
  if( nullptr != m_pEditors ) {
    GEditor* pEditor = (*m_pEditors)[0];
    if( nullptr != pEditor ) {
      value = static_cast<uint32_t>(pEditor->getValue());
    }
  }

  // Validate or modify password
  DevReg_Write(m_uRegNum, value);

  GDialog::accept();
}
//-----------------------------------------------------------------------------
