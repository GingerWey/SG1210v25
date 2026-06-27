//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CSGDraw.h
 Version     : V1.02
 By          : Wey. Silver Grid

 Description : CSG image drawing — MCU-safe streaming decoder integration.
               Uses CsgDecodeInit / CsgDecodePixels (no dynamic vectors).
               Memory allocated from emWin GUIHEAP, freed after drawing.
               Supports atlas sub-pictures and palette replacement.

 Date        : 2026.06.26 (V1.02 — unified streaming path, VxARGB FromCrm/ToCrm)
              2026.06.25 (V1.01 — added picIndex & saturation params)
              2026.06.24 (V1.00 — initial CSG drawing API)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_CSGDRAW_H
#define GUI_CSGDRAW_H

#include "GUIPicture.h"
#include <cstdint>

//-----------------------------------------------------------------------------
// Line buffer size -- must be ≥ max image width × max bytes-per-color (4 for RGBA)
// 256 pixels × 4 bytes = 1024 bytes
//-----------------------------------------------------------------------------
#ifndef CSG_LINEBUF_SIZE
  #define CSG_LINEBUF_SIZE  1024
#endif

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Public API
//=============================================================================

/// Draw a CSG image (unified Sim+MCU path via streaming decoder).
/// Output buffer allocated from GUI_ALLOC in CRM format.
/// Sim: CRM→RGBA→GUI_DrawPixel.  MCU: CRM→LCD direct write.
///
/// @param pPic       TGUIPicture with Type=ID_CSG, pData=CSG bytes, Size=byte count
/// @param x0         Left coordinate on LCD
/// @param y0         Top coordinate on LCD
/// @param picIndex   0-based sub-picture index within an atlas (default 0)
/// @param saturation Saturation percentage 10–100 (default 100 = no change)
void CSG_DrawPicture(const TGUIPicture* pPic, int x0, int y0,
                     int picIndex = 0, int saturation = 100);

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif  // GUI_CSGDRAW_H
