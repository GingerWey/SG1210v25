//-----------------------------------------------------------------------------
/*
 File        : GDialogRes.h
 Version     : V1.00
 By          : Wey@SilverGrid

 Description : Dialog resource configurations for GConfigDialog
               Public accessor functions for dialog configs

 Date        : 2026.07.21
*/
//-----------------------------------------------------------------------------
#ifndef GDIALOGRES_H
#define GDIALOGRES_H

#include "GDialog.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// Public Functions
//=============================================================================

// Get GNumRegDialog configuration
const GDialog::GDialogConfig* GetNumRegDialogConfig();

// Get GLoginDialog configuration
const GDialog::GDialogConfig* GetLoginDialogConfig();

// Get GIPAddressDialog configuration
const GDialog::GDialogConfig* GetIPAddressDialogConfig();

// Get GDatetimeDialog configuration
const GDialog::GDialogConfig* GetDatetimeDialogConfig();

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // GDIALOGRES_H
