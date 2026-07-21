//-----------------------------------------------------------------------------
/*
 File        : GDatetimeDialog.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GDatetimeDialog - date/time editor (stub implementation).
               Per SG1210v25 dialog spec section 8.

 Date        : 2026.07.21 (V1.00 - stub implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GDatetimeDialog.h"
#include "GEditorPanel.h"
#include "DevIntf.h"
#include <cstring>

//-----------------------------------------------------------------------------
GDatetimeDialog::GDatetimeDialog(const GDialogConfig* pConfig, const void* Param)
  : GDialog(pConfig, Param)
{
}

//-----------------------------------------------------------------------------
TDateTimeType GDatetimeDialog::getResult()
{
  TDateTimeType dt;
  memset(&dt, 0, sizeof(dt));

  if( nullptr != m_pEditors ) {
    // Extract 6 editors: Year, Month, Day, Hour, Minute, Second
    GEditor* pEditor = nullptr;
    if( nullptr != (pEditor = (*m_pEditors)[0]) ) { dt.Year    = static_cast<uint16_t>(pEditor->getValue()); }
    if( nullptr != (pEditor = (*m_pEditors)[1]) ) { dt.Month   = static_cast<uint8_t>(pEditor->getValue()); }
    if( nullptr != (pEditor = (*m_pEditors)[2]) ) { dt.Day     = static_cast<uint8_t>(pEditor->getValue()); }
    if( nullptr != (pEditor = (*m_pEditors)[3]) ) { dt.Hours   = static_cast<uint8_t>(pEditor->getValue()); }
    if( nullptr != (pEditor = (*m_pEditors)[4]) ) { dt.Minutes = static_cast<uint8_t>(pEditor->getValue()); }
    if( nullptr != (pEditor = (*m_pEditors)[5]) ) { dt.Seconds = static_cast<uint8_t>(pEditor->getValue()); }
  }

  return dt;
}
//-----------------------------------------------------------------------------
