//-----------------------------------------------------------------------------
/*
 File        : GConfigForm.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : Config form - parameter configuration form.
               Shared by 4 config menu items (Logic / Device / Serial /
               Ethernet). Displays configuration registers grouped by config
               type, browsable by category. Style aligned with LogListForm
               (outer/inner frames + right scrollbar + top Caption).
               Per SG1210v25 form design spec section VI.

               Init argument selects the initial config type (TConfigType,
               passed as (const void*)(uintptr_t)cgtXxx). Defaults to cgtLogic.

 Date        : 2026.07.13 (V1.00 - initial implementation, Display mode only
              per spec 6.7 - editor dialogs not yet designed)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GCONFIGFORM_H
#define GUI_GCONFIGFORM_H

#include "GWinTypes.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// Config type (spec table 6-1). Passed to _Init as (const void*)(uintptr_t).
//-----------------------------------------------------------------------------
typedef enum tagConfigType {
  cgtLogic    = 0,      // Logic-Config    (idCfgGroup01)
  cgtDevice   = 1,      // Device-Config   (idCfgGroup02)
  cgtSerial   = 2,      // Serial-Config   (idCfgGroup03)
  cgtEthernet = 3       // Ethernet-Config (idCfgGroup04)
} TConfigType;

//=============================================================================
// Global data
//-----------------------------------------------------------------------------
extern const GWinForm FConfigForm;
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // GUI_GCONFIGFORM_H
