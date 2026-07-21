//-----------------------------------------------------------------------------
/*
 File        : GNumRegDialog.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GNumRegDialog - numeric register editor implementation.
               Per SG1210v25 dialog spec section 5.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GEditorPanel.h"
#include "GNumRegDialog.h"

#include "DevIntf.h"
#include "GDialog.h"
#include "Strings/TextStrs.h"
#include <cstdint>

//-----------------------------------------------------------------------------
GNumRegDialog::GNumRegDialog(const GDialogConfig* pConfig, const void* Param)
  : GDialog(pConfig, Param)
{
  m_uRegNum = *reinterpret_cast<const uint32_t*>(Param);
  m_pProperty = DevIntf_GetRegInfo(m_uRegNum);
}

//-----------------------------------------------------------------------------
void GNumRegDialog::setRegNum(uint32_t uRegNum)
{
  m_uRegNum = uRegNum;
  m_pProperty = DevIntf_GetRegInfo(m_uRegNum);
}

//-----------------------------------------------------------------------------
uint32_t GNumRegDialog::getResult() const
{
  if( nullptr != m_pEditors ) {
    GEditor* pEditor = (*m_pEditors)[0];
    if( nullptr != pEditor ) {
      return static_cast<uint32_t>(pEditor->getValue());
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
const char* GNumRegDialog::getTitle() const
{
  if( nullptr != m_pProperty ) {
    return GetMultiLangString(m_pProperty->NameStrId);
  }
  return nullptr;
}
//-----------------------------------------------------------------------------
