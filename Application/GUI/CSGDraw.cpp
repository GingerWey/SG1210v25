//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CSGDraw.cpp
 Version     : V1.10
 By          : Wey. Silver Grid

 Description : CSG image drawing — unified Sim+MCU streaming path.
               Row-batch bitmap output: GUI_DrawBitmapExp (Sim) / FSMC burst (MCU).
               pClipRect honored on BOTH Sim and MCU (nullptr=full screen, &rect=clipped).
               Does NOT depend on emWin GUI_SetClipRect/GUI_GetClipRect.

 Date        : 2026.07.09 (V1.10 — Sim path now honors pClipRect via emWin GUI_SetClipRect
                          (set around the decode loop). Previously Sim ignored pClipRect,
                          so a clipped background redraw overwrote the whole screen,
                          erasing non-redrawn content. MCU path unchanged (manual clip).
                          Affected GPDataListForm refresh and GPMenuForm _EraseSelArea.)
              2026.07.02 (V1.09 — strict Yoda conditions & mandatory braces for if/for/while)
              2026.07.02 (V1.08 — v1.6 CRN bit7: crnReal=kCrnColorCountMask, hasTransparency=kCrnTransparentFlag)
              2026.07.02 (V1.07 — Yoda-style + brace-style compliance)
              2026.06.30 (V1.06 — pClipRect parameter replaces GUI_GetClipRect)
              2026.06.30 (V1.05 — MCU software clip rect via GUI_GetClipRect, deprecated)
              2026.06.26 (V1.04 — performance: row-bitmap draw, inline CRM→RGB565)
*/
//-----------------------------------------------------------------------------
#include "CSGDraw.h"

#include "CSGCommon.h"
#include "CSGDecoder.h"    // CsgDecodeInit, CsgDecodePixels
#include "CSGCodec.h"

#include "GUIBitmap.h"
#include "DevRegs.h"

#include "GUI.h"
#include "LCD.h"
#include "GUI_Private.h"

#ifndef __vmSIMULATOR__
  #include "ST7789S.h"
#endif

#include <Global.h>
#include <LCD.h>
#include <stdint.h>
#include <string.h>
#include <LCD_Protected.h>
#include <GUI_Type.h>

//=============================================================================
// Local constants
//-----------------------------------------------------------------------------
#ifndef LCD_SetCursor
  #define LCD_SetCursor(x, y)  LCD_ST7789S_SetCursor(x, y)
#endif

#ifndef LCD_WRAM_Prepare
  #define LCD_WRAM_Prepare()   (*(volatile uint16_t*)0x60000000)
#endif

//-----------------------------------------------------------------------------
/// Decode a CSG picture header from the compressed data blob.
/// Returns true on success, filling *picOut and setting *dataOut to the
/// start of compressed pixel data.
//-----------------------------------------------------------------------------
static bool ParseCsgPicture(const uint8_t* csgData,
                            size_t csgSize,
                            int picIndex,
                            CSGPicture* picOut,
                            const uint8_t** dataOut,
                            size_t* dataSizeOut)
{
    if (nullptr == csgData || kCsgPictureHeaderSize > csgSize) {
        return false;
    }

    const uint8_t* ptr = csgData;

    // Check if this is a CSG atlas (starts with "CG") or a standalone picture ("Vx")
    if (kCsgGlobalMagic[0] == ptr[0] && kCsgGlobalMagic[1] == ptr[1]) {
        // Atlas mode: read global header, then extract sub-picture
        const CSGHeader* hdr = reinterpret_cast<const CSGHeader*>(ptr);

        int count = static_cast<int>(hdr->GetPicCount());
        if (0 == count) {
            // Empty atlas with embedded picture at offset 0
            if (csgSize < static_cast<size_t>(kCsgGlobalHeaderFixedSize + kCsgPictureHeaderSize)) {
                return false;
            }
            ptr += kCsgGlobalHeaderFixedSize;
            memcpy(picOut, ptr, kCsgPictureHeaderSize);
            *dataOut    = ptr + kCsgPictureHeaderSize;
            *dataSizeOut = csgSize - kCsgGlobalHeaderFixedSize - kCsgPictureHeaderSize;
            return true;
        }

        // Resolve sub-picture by index
        if (0 > picIndex || count <= picIndex) {
            return false;
        }

        uint32_t offset = hdr->GetPicOffset(picIndex);
        if (0 == offset || offset + kCsgPictureHeaderSize > csgSize) {
            return false;
        }

        ptr = csgData + offset;
    }

    // Read picture header (either standalone or extracted from atlas)
    memcpy(picOut, ptr, kCsgPictureHeaderSize);

    // Validate picture magic
    if (kCsgPictureMagic[0] != picOut->magic[0] ||
        kCsgPictureMagic[1] != picOut->magic[1]) {
        return false;
    }

    *dataOut     = ptr + kCsgPictureHeaderSize;
    *dataSizeOut = picOut->size - kCsgPictureHeaderSize;
    return true;

}

//-----------------------------------------------------------------------------
/// Draw one decoded RGBA pixel at (x, y), handling transparency.
/// MCU path:  writes RGB565 directly to LCD via FSMC.
/// Sim path:  uses GUI_SetColor + GUI_DrawPixel.
//-----------------------------------------------------------------------------
static inline void DrawPixel(int x, int y,
                             uint8_t red, uint8_t green,
                             uint8_t blue, uint8_t alpha)
{
    if (0 == alpha) {
        return;
    }

#ifndef __vmSIMULATOR__
    // MCU: convert RGB888 -> RGB565 and write directly to LCD
    uint16_t rgb565 = ((uint16_t)(red   >> 3) << 11) |
                      ((uint16_t)(green >> 2) <<  5) |
                      ((uint16_t)(blue  >> 3));
    LCD_SetCursor(x, y);
    LCD_WRAM_Prepare();
    *(volatile uint16_t*)0x60000000 = rgb565;  // FSMC bank 1, region 1
#else
    // Simulator: use emWin pixel drawing
    uint32_t color = ((uint32_t)red   << 16) |
                     ((uint32_t)green <<  8) |
                      (uint32_t)blue;
    GUI_SetColor(color);
    GUI_DrawPixel(x, y);
#endif
}

//=============================================================================
// Saturation helpers
//=============================================================================

/// Apply saturation to a CRM-encoded palette (in-place).
/// Uses VxARGB::FromCrm → ApplySaturation → ToCrm for clean round-trip.
static void SaturatePalette(uint8_t* palBytes, int entryCount, int bpc,
                            uint8_t colorMode, int sat)
{
    if (100 <= sat || nullptr == palBytes || 0 >= entryCount) { return; }
    if (0 > sat) { sat = 0; }

    ColorMode cm = static_cast<ColorMode>(colorMode);
    for (int i = 0; i < entryCount; ++i) {
        VxARGB px = VxARGB::FromCrm(palBytes + i * bpc, cm);
        px.ApplySaturation(sat);
        px.ToCrm(palBytes + i * bpc, cm);
    }
}

//=============================================================================
// Unified CSG drawing (streaming decoder, Sim + MCU single path)
//=============================================================================

// ---- Fast inline helpers ----

/// CRM → RGB565 for common color modes (hot path, no VxARGB overhead)
static inline uint16_t CrmToRgb565(const uint8_t* crm, uint8_t colorMode) {
    switch (colorMode) {
    case 0x21: // RGB565 — direct copy
        return static_cast<uint16_t>(crm[0]) | (static_cast<uint16_t>(crm[1]) << 8);
    case 0x22: { // BGR565 — swap to RGB565
        uint16_t v = static_cast<uint16_t>(crm[0]) | (static_cast<uint16_t>(crm[1]) << 8);
        uint8_t b = static_cast<uint8_t>((v >> 8) & 0xF8);
        uint8_t g = static_cast<uint8_t>((v >> 3) & 0xFC);
        uint8_t r = static_cast<uint8_t>((v << 3) & 0xF8);
        return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (uint16_t)(b >> 3);
    }
    case 0x33: // RGB888 → RGB565
        return ((uint16_t)(crm[0] >> 3) << 11) | ((uint16_t)(crm[1] >> 2) << 5) | (uint16_t)(crm[2] >> 3);
    case 0x41: // RGB8888 → RGB565 (skip transparent)
        if (0 == crm[3]) { return 0xF81F; }  // magenta sentinel → skip
        return ((uint16_t)(crm[0] >> 3) << 11) | ((uint16_t)(crm[1] >> 2) << 5) | (uint16_t)(crm[2] >> 3);
    default: {
        VxARGB px = VxARGB::FromCrm(crm, static_cast<ColorMode>(colorMode));
        return ((uint16_t)(px.r >> 3) << 11) | ((uint16_t)(px.g >> 2) << 5) | (uint16_t)(px.b >> 3);
    }
    }
}

/// RGB565 → Sim 32-bit GUI_COLOR (GUICC_M8888I: 0x00RRGGBB)
static inline uint32_t Rgb565ToSim(uint16_t v) {
    uint32_t r = (v >> 8) & 0xF8;
    uint32_t g = (v >> 3) & 0xFC;
    uint32_t b = (v << 3) & 0xF8;
    return (r << 16) | (g << 8) | b;  // 0x00RRGGBB
}

/// Saturate a single RGB565 pixel (in-place) for CRN=0 true-color path
static inline uint16_t SaturateRgb565(uint16_t v, int sat) {
    int r = (v >> 11) & 0x1F;
    int g = (v >> 5) & 0x3F;
    int b = v & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    int gray = (r * 299 + g * 587 + b * 114) / 1000;
    int factor = sat * 10;
    r = (gray * (1000 - factor) + r * factor) / 1000;
    g = (gray * (1000 - factor) + g * factor) / 1000;
    b = (gray * (1000 - factor) + b * factor) / 1000;
    return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (uint16_t)(b >> 3);
}

//=============================================================================
// Draw CSG Picture
//=============================================================================
// CSG_DrawPicture — Unified Sim+MCU streaming draw
//
// MCU path writes directly to LCD via FSMC, bypassing emWin entirely.
// Therefore emWin's GUI_SetClipRect/GUI_GetClipRect cannot be used.
// Instead, clip is controlled by the pClipRect parameter:
//   pClipRect = nullptr  → draw full image (default, zero overhead)
//   pClipRect = &rect    → draw only pixels within rect
//
// Callers that need partial redraw (e.g. _EraseSelArea in GPMenuForm)
// pass an explicit clip rect.  All other callers omit the parameter and
// get full-screen drawing with no clipping overhead.
//=============================================================================

void CSG_DrawPicture(const TGUIPicture* pPic, int x0, int y0,
                     int picIndex, int saturation, const GUI_RECT* pClipRect)
{

    if (nullptr == pPic || pPic->pData == nullptr || pPic->Type != ID_CSG) {
        return;
    }

    if (10 > saturation) {
        saturation = 10;
    }
    if (100 < saturation) {
        saturation = 100;
    }

    const uint8_t* csgData = static_cast<const uint8_t*>(pPic->pData);

    // 1. Parse picture header (handles atlas sub-pictures)
    CSGPicture    pic;
    const uint8_t* compData = nullptr;
    size_t         compSize = 0;

    if (false == ParseCsgPicture(csgData, pPic->Size, picIndex,
                                 &pic, &compData, &compSize)) {
        return;
    }

    int bpc    = pic.BytesPerColor();
    int crn    = pic.crn;
    int crnReal = crn & kCrnColorCountMask;           // v1.6: actual color count (mask bit7)
    bool hasTransparency = (0 < crnReal
                            && 0 == (crn & kCrnTransparentFlag)); // v1.6: bit7=0 → transparent
    int totalPixels = pic.width * pic.height;
    int width  = pic.width;
    ColorMode outMode = static_cast<ColorMode>(pic.colorMode);
    uint8_t colorMode = pic.colorMode;

    // 2. Read embedded palette (v1.6: use crnReal, not crn)
    const uint8_t* embPalette = nullptr;
    if (0 < crnReal && 0 < pic.ppos
        && pic.ppos + static_cast<uint16_t>(crnReal * bpc) <= pic.size) {
        embPalette = compData + pic.ppos - kCsgPictureHeaderSize;
    }

    // 3. Apply saturation to palette (v1.6: use crnReal)
    uint8_t* satPalBuf = nullptr;
    GUI_HMEM hPalBuf = 0;
    const uint8_t* activePalette = embPalette;
    const bool doSat = (100 != saturation);
    if (true == doSat && nullptr != embPalette && 0 < crnReal) {
        int palBytes = crnReal * bpc;
        hPalBuf = GUI_ALLOC_AllocZero(palBytes);
        if (0 != hPalBuf) {
            satPalBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hPalBuf));
            memcpy(satPalBuf, embPalette, palBytes);
            SaturatePalette(satPalBuf, crnReal, bpc, colorMode, saturation);
            activePalette = satPalBuf;
        }
    }

    // 4. Precompute transparent color in RGB565 (palette[0] → RGB565)
    //    hasTransparency was computed at line 261 (v1.6 CRN bit7 check)
    uint16_t transpRgb565 = 0x0000;
    if (true == hasTransparency) {
        transpRgb565 = CrmToRgb565(activePalette, colorMode);
    }

    // 5. Allocate buffers: CRM output + bitmap (RGB565 for MCU, 32-bit for Sim)
    int maxBatch = width;
    if (totalPixels < maxBatch) {
        maxBatch = totalPixels;
    }
    int crmBytes = maxBatch * bpc;
#ifdef __vmSIMULATOR__
    int bmpBytes = maxBatch * 4;  // 32-bit 0x00RRGGBB for GUICC_M8888I
#else
    int bmpBytes = maxBatch * 2;  // RGB565 for ST7789S
#endif
    GUI_HMEM hOutBuf = GUI_ALLOC_AllocZero(crmBytes);
    GUI_HMEM hBmpBuf = GUI_ALLOC_AllocZero(bmpBytes);
    if (0 == hOutBuf || 0 == hBmpBuf) {
        if (0 != hOutBuf) {
            GUI_ALLOC_Free(hOutBuf);
        }
        if (0 != hBmpBuf) {
            GUI_ALLOC_Free(hBmpBuf);
        }
        if (0 != hPalBuf) {
            GUI_ALLOC_Free(hPalBuf);
        }
        return;
    }
    uint8_t* outBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hOutBuf));
    void*    bmpBuf = GUI_ALLOC_h2p(hBmpBuf);

    // 6. Init streaming decoder
    //    Static allocation: CSGDecoderState contains an 8KB window[] → won't fit
    //    on any RTOS task stack (HMITask=8KB).  Placed in .bss to avoid overflow.
    static CSGDecoderState state;
    if (CsgDecodeInit(&state, &pic, outBuf, activePalette, outMode)
        != CSG_ErrCode::kOk) {
        GUI_ALLOC_Free(hOutBuf); 
        GUI_ALLOC_Free(hBmpBuf);
        if (0 != hPalBuf) {
            GUI_ALLOC_Free(hPalBuf);
        }
        return;
    }
    state.stream     = compData + (pic.dpos - kCsgPictureHeaderSize);
    state.streamSize = compSize - (pic.dpos - kCsgPictureHeaderSize);

#ifdef __vmSIMULATOR__
    // Sim: emWin honors GUI_SetClipRect for all drawing primitives (WIN32
    // driver). Set it once around the decode loop so a clipped redraw only
    // touches pClipRect — no manual row/column slicing needed. Restored to
    // nullptr after the loop. MCU does NOT use this (CSG bypasses emWin).
    if (nullptr != pClipRect) {
        GUI_SetClipRect(pClipRect);
    }
#endif

    // 7. Decode + draw loop — row-at-a-time via bitmap
    int row = 0;
    while (state.pixelsDecoded < totalPixels) {
        int prevDecoded = state.pixelsDecoded;
        CSG_ErrCode decErr = CsgDecodePixels(&state, outBuf, maxBatch);
        int n = state.pixelsDecoded - prevDecoded;
        if (CSG_ErrCode::kOk != decErr || 0 == n) {
            break;
        }

#ifdef __vmSIMULATOR__
        // ---- Sim: hybrid draw ----
        //   Clipping to pClipRect is handled by emWin via GUI_SetClipRect (set
        //   around the loop below) — efficient, and correct on Sim where the
        //   WIN32 driver honors the clip for GUI_DrawBitmapExp / GUI_DrawPixel.
        //   MCU cannot use this (CSG bypasses emWin) → MCU path clips manually.
        //   Rows with transparent pixels: per-pixel draw, skip transparent.
        //   Rows without transparent pixels: fast GUI_DrawBitmapExp.
        if (true == hasTransparency) {
            uint32_t transp32 = Rgb565ToSim(transpRgb565);
            bool bHasTransp = false;
            for (int i = 0; i < n && false == bHasTransp; ++i) {
                if (transp32 == Rgb565ToSim(CrmToRgb565(outBuf + i * bpc, colorMode))) {
                    bHasTransp = true;
                }
            }
            if (true == bHasTransp) {
                // Slow path: per-pixel draw, skip transparent → background shows through
                for (int i = 0; i < n; ++i) {
                    uint16_t v565 = CrmToRgb565(outBuf + i * bpc, colorMode);
                    if (v565 == transpRgb565) {
                        continue; // skip transparent
                    }
                    uint32_t c = Rgb565ToSim(v565);
                    GUI_SetColor(c & 0x00FFFFFF);  // strip alpha, keep RGB
                    GUI_DrawPixel(x0 + i, y0 + row);
                }
            } else {
                // Fast path: no transparency in this row → bitmap draw
                uint32_t* bmp32 = static_cast<uint32_t*>(bmpBuf);
                if (true == doSat && 0 == crn) {
                    for (int i = 0; i < n; ++i) {
                        bmp32[i] = Rgb565ToSim(SaturateRgb565(
                            CrmToRgb565(outBuf + i * bpc, colorMode), saturation));
                    }
                } else {
                    for (int i = 0; i < n; ++i) {
                        bmp32[i] = Rgb565ToSim(CrmToRgb565(outBuf + i * bpc, colorMode));
                    }
                }
                GUI_DrawBitmapExp(x0, y0 + row, n, 1, 1, 1, 32, n * 4,
                                  static_cast<const U8*>(bmpBuf), nullptr);
            }
        } else if (true == doSat && 0 == crn) {
            uint32_t* bmp32 = static_cast<uint32_t*>(bmpBuf);
            for (int i = 0; i < n; ++i) {
                bmp32[i] = Rgb565ToSim(SaturateRgb565(
                    CrmToRgb565(outBuf + i * bpc, colorMode), saturation));
            }
            GUI_DrawBitmapExp(x0, y0 + row, n, 1, 1, 1, 32, n * 4,
                              static_cast<const U8*>(bmpBuf), nullptr);
        } else {
            uint32_t* bmp32 = static_cast<uint32_t*>(bmpBuf);
            for (int i = 0; i < n; ++i) {
                bmp32[i] = Rgb565ToSim(CrmToRgb565(outBuf + i * bpc, colorMode));
            }
            GUI_DrawBitmapExp(x0, y0 + row, n, 1, 1, 1, 32, n * 4,
                              static_cast<const U8*>(bmpBuf), nullptr);
        }
#else
        // ---- MCU: CRM → RGB565 (with software clip rect) ----
        // Always enforce GUI_GetClipRect — every pixel write is clipped.

        if( nullptr != pClipRect ) {
            // Skip row entirely if outside clip Y range
            if (y0 + row < pClipRect->y0 ) {
                ++row;
                continue;
            } else if(y0 + row > pClipRect->y1) {
                break;
            }

            // Clipped column range for burst-path rows
            int iClipStart = (pClipRect->x0 > x0) ? (pClipRect->x0 - x0) : 0;
            int iClipEnd   = (pClipRect->x1 < x0 + n - 1) ? (pClipRect->x1 - x0 + 1) : n;
            if (iClipStart >= iClipEnd) {
                ++row;
                continue;
            }

            const uint16_t* puwPixels = (uint16_t*)(outBuf);
            if (true == hasTransparency) {
                // Scan for transparent pixels in this row
                bool bHasTransp = false;
                for (int i = 0; i < n && false == bHasTransp; ++i) {
                    if( transpRgb565 == puwPixels[i] ) {
                        bHasTransp = true;
                    }
                }

                if (true == bHasTransp) {
                    // Per-pixel: skip transparent + clipped pixels
                    U32 bNeedCursor = 1;
                    for (int i = 0; i < n; ++i) {
                        U16 uwColor = *puwPixels++;
                        int  sx      = x0 + i;
        
                        if (sx < pClipRect->x0 || sx > pClipRect->x1) {
                           continue;
                        }

                        if (transpRgb565 == uwColor) {
                           bNeedCursor = 1;
                           continue;
                        }

                        if (0 != bNeedCursor) {
                            bNeedCursor = 0;
                            LCD_SetCursor(sx, y0 + row);
                        }
                        LCD_DATADDR = uwColor;
                    }
                } else {
                    // Fast burst — clipped range only
                    LCD_SetCursor(x0 + iClipStart, y0 + row);
                    puwPixels += iClipStart;
                    for (int i = iClipStart; i < iClipEnd; ++i) {
                        LCD_DATADDR = *puwPixels++;
                    }
                    puwPixels += (n - iClipEnd);
                }
            } else if (true == doSat && 0 == crn) {
                LCD_SetCursor(x0 + iClipStart, y0 + row);
                puwPixels += iClipStart;
                for (int i = iClipStart; i < iClipEnd; ++i) {
                    LCD_DATADDR = SaturateRgb565(*puwPixels++, saturation);
                }
            } else {
                LCD_SetCursor(x0 + iClipStart, y0 + row);
                puwPixels += iClipStart;
                for (int i = iClipStart; i < iClipEnd; ++i) {
                    LCD_DATADDR = *puwPixels++;
                }
            }
        } else {
            // No Clip rect.
            const uint16_t* puwPixels = (uint16_t*)(outBuf);
            if (true == hasTransparency) {
                // Scan for transparent pixels in this row
                bool bHasTransp = false;
                for (int i = 0; i < n && false == bHasTransp; ++i) {
                    if( transpRgb565 == puwPixels[i] ) {
                        bHasTransp = true;
                    }
                }

                if (true == bHasTransp) {
                    // Per-pixel: skip transparent + clipped pixels
                    U32 bNeedCursor = 1;
                    for (int i = 0; i < n; ++i) {
                        U16 uwColor = *puwPixels++;
                        if (transpRgb565 == uwColor) {
                          bNeedCursor = 1;
                          continue;
                        }

                        if (0 != bNeedCursor) {
                            bNeedCursor = 0;
                            LCD_SetCursor(x0 + i, y0 + row);
                        }

                        LCD_DATADDR = uwColor;
                    }
                } else {
                    // Fast path: no transparent pixels in this row
                    LCD_SetCursor(x0, y0 + row);
                    for (int i = 0; i < n; ++i) {
                        LCD_DATADDR = *puwPixels++;
                    }
                }
            } else if (true == doSat && 0 == crn) {
                LCD_SetCursor(x0, y0 + row);
                for (int i = 0; i < n; ++i) {
                    LCD_DATADDR = SaturateRgb565(*puwPixels++, saturation);
                }
            } else {
                LCD_SetCursor(x0, y0 + row);
                for (int i = 0; i < n; ++i) {
                    LCD_DATADDR = *puwPixels++;
                }
            }
          }
#endif
        ++row;
          
          
#ifdef RRS_GUITASK
        SetRSTSrc(RRS_GUITASK);
#endif
    }

#ifdef __vmSIMULATOR__
    // Restore emWin clip to default (no clip)
    if (nullptr != pClipRect) {
        GUI_SetClipRect(nullptr);
    }
#endif

    // Free DEFLATE intermediate buffer if allocated
    if (0 != state.deflateBufHandle) {
        GUI_ALLOC_Free(static_cast<GUI_HMEM>(state.deflateBufHandle));
    }

    GUI_ALLOC_Free(hOutBuf);
    GUI_ALLOC_Free(hBmpBuf);
    if (0 != hPalBuf) { 
       GUI_ALLOC_Free(hPalBuf); 
    }
}
