//-----------------------------------------------------------------------------
/*
 File        : GIPAddressDialog.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GIPAddressDialog - IPv4 address editor (stub implementation).
               Per SG1210v25 dialog spec section 7.

 Date        : 2026.07.21 (V1.00 - stub implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "DevIntf.h"
#include "GDialog.h"
#include "GEditorPanel.h"
#include "GIPAddressDialog.h"
#include "Strings/TextStrs.h"
#include <DevTypes.h>
#include <cstdint>

//-----------------------------------------------------------------------------
GIPAddressDialog::GIPAddressDialog(const GDialogConfig* pConfig, const void* Param)
  : GDialog(pConfig, Param)
{
  m_uRegNum = *reinterpret_cast<const uint32_t*>(Param);
}

//-----------------------------------------------------------------------------
void GIPAddressDialog::setRegNum(uint32_t uRegNum)
{
  m_uRegNum = uRegNum;
}

//-----------------------------------------------------------------------------
uint32_t GIPAddressDialog::getResult() const
{
  // Pack 4 octets into uint32_t (oct0<<24)|(oct1<<16)|(oct2<<8)|oct3
  if( nullptr != m_pEditors ) {
    uint32_t result = 0;
    for( int i = 0; i < 4; i++ ) {
      GEditor* pEditor = (*m_pEditors)[i];
      if( nullptr != pEditor ) {
        result |= (pEditor->getValue() & 0xFF) << ((3 - i) * 8);
      }
    }
    return result;
  }
  return 0;
}

//-----------------------------------------------------------------------------
const char* GIPAddressDialog::getTitle() const
{
  const TDevRegInfoItem* pProp = DevIntf_GetRegInfo(m_uRegNum);
  if( nullptr != pProp ) {
    return GetMultiLangString(pProp->NameStrId);
  }
  return nullptr;
}
//-----------------------------------------------------------------------------
