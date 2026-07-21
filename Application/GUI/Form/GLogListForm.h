//-----------------------------------------------------------------------------
/*
 File        : GLogListForm.h
 Version     : V1.02
 By          : Wey. Silver Grid

 Description : LogList form — log query form.
               Displays device event/alarm/fault logs from EVTMGR, grouped by
               category (mltDeviceLog / mltDevStatus / mltAutoCtrl). Style aligned with
               DataListForm (frames + scrollbar) plus a top Caption showing the
               category name. Per SG1210v25 form design spec section V.

 Date        : 2026.07.11 (V1.02 — fix category switch: clear stale rows, LEFT/RIGHT
              ignore key-repeat, Caption redraw inset to avoid outer-frame conflict)
              2026.07.11 (V1.01 — optimize category switch: partial redraw only)
              2026.07.10 (V1.00 — initial implementation)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GPLOGLISTFORM_H
#define GUI_GPLOGLISTFORM_H

#include "GWinTypes.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
extern const GWinForm FLogListForm;
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif  // GUI_GPLOGLISTFORM_H
