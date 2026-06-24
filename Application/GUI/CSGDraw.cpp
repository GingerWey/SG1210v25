//-----------------------------------------------------------------------------
/*
 File        : CSGDraw.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : CSG image drawing -- MCU-safe implementation.
               Follows the LCDX_Bitmap_Draw pattern with dual MCU / simulator
               drawing paths.  Uses streaming decoder (zero heap vectors).

 Date        : 2026.06.24
*/
//-----------------------------------------------------------------------------
#include "CSGDraw.h"

#include "CSGCommon.h"
#include "CSGDecoder.h"    // CsgDecodeInit, CsgDecodePixels
#include "CSGCodec.h"

#include "GUIBitmap.h"

#include "GUI.h"
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
    if (csgData == nullptr || csgSize < kCsgPictureHeaderSize)
        return false;

    const uint8_t* ptr = csgData;

    // Check if this is a CSG atlas (starts with "CG") or a standalone picture ("Vx")
    if (ptr[0] == kCsgGlobalMagic[0] && ptr[1] == kCsgGlobalMagic[1]) {
        // Atlas mode: read global header, then extract sub-picture
        const CSGHeader* hdr = reinterpret_cast<const CSGHeader*>(ptr);

        int count = static_cast<int>(hdr->GetPicCount());
        if (count == 0) {
            // Empty atlas with embedded picture at offset 0
            if (csgSize < static_cast<size_t>(kCsgGlobalHeaderFixedSize + kCsgPictureHeaderSize))
                return false;
            ptr += kCsgGlobalHeaderFixedSize;
            memcpy(picOut, ptr, kCsgPictureHeaderSize);
            *dataOut    = ptr + kCsgPictureHeaderSize;
            *dataSizeOut = csgSize - kCsgGlobalHeaderFixedSize - kCsgPictureHeaderSize;
            return true;
        }

        // Resolve sub-picture by index
        if (picIndex < 0 || picIndex >= count)
            return false;

        uint32_t offset = hdr->GetPicOffset(picIndex);
        if (offset == 0 || offset + kCsgPictureHeaderSize > csgSize)
            return false;

        ptr = csgData + offset;
    }

    // Read picture header (either standalone or extracted from atlas)
    memcpy(picOut, ptr, kCsgPictureHeaderSize);

    // Validate picture magic
    if (picOut->magic[0] != kCsgPictureMagic[0] ||
        picOut->magic[1] != kCsgPictureMagic[1])
        return false;

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
    if (alpha == 0) return;

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

//-----------------------------------------------------------------------------
/// Convert CRM-encoded source pixel to RGBA, applying optional palette override.
/// src: pointer to CRM-encoded pixel data (src[0..bpc-1])
/// bpc: bytes per color (2=RGB565, 3=RGB888, 4=RGB8888)
/// colorMode: the ColorMode enum value
/// palette: CRM palette bytes, or nullptr for true-color
/// crn: palette size (0=true-color)
/// outRed/Green/Blue/Alpha: output
//-----------------------------------------------------------------------------
/// Convert a single uncompressed CRM pixel to RGBA.
/// Caller ensures src >= bpc bytes.
static void CrmPixelToRgba(const uint8_t* src,
                           uint8_t colorMode,
                           uint8_t* outRed, uint8_t* outGreen,
                           uint8_t* outBlue, uint8_t* outAlpha)
{
    uint8_t r = 0, g = 0, b = 0, a = 255;
    switch (colorMode) {
    case 0x21: { // RGB565
        uint16_t v = src[0] | (src[1] << 8);
        r = ((v >> 11) & 0x1F) * 255 / 31;
        g = ((v >>  5) & 0x3F) * 255 / 63;
        b = (v & 0x1F) * 255 / 31;
        break;
    }
    case 0x22: { // BGR565
        uint16_t v = src[0] | (src[1] << 8);
        b = ((v >> 11) & 0x1F) * 255 / 31;
        g = ((v >>  5) & 0x3F) * 255 / 63;
        r = (v & 0x1F) * 255 / 31;
        break;
    }
    case 0x31: // RGB666
        r = src[0] * 255 / 63; g = src[1] * 255 / 63; b = src[2] * 255 / 63;
        break;
    case 0x33: // RGB888
        r = src[0]; g = src[1]; b = src[2];
        break;
    case 0x41: // RGB8888
        r = src[0]; g = src[1]; b = src[2]; a = src[3];
        break;
    default: break;
    }
    *outRed = r; *outGreen = g; *outBlue = b; *outAlpha = a;
}

//=============================================================================
// Public API
//=============================================================================

void CSG_DrawPicture(const TGUIPicture* pPic, int x0, int y0)
{
    CSG_DrawPictureEx(pPic, 0, nullptr, x0, y0);
}

void CSG_DrawPictureEx(const TGUIPicture* pPic,
                       int picIndex,
                       const uint8_t* extPalette,
                       int x0, int y0)
{
    if (pPic == nullptr || pPic->pData == nullptr) return;
    if (pPic->Type != ID_CSG) return;

    const uint8_t* csgData = static_cast<const uint8_t*>(pPic->pData);

    // Simulator: use full decoder (handles all CAS modes)
    // MCU: will use streaming decoder (CAS 0/1/3 only)
#ifdef __vmSIMULATOR__
    CSGDecoder decoder;
    DecoderResult r = decoder.DecodePicture(csgData, pPic->Size);
    if (r.error != CSG_ErrCode::kOk || r.pixels.empty()) return;

    const uint8_t* px = r.pixels.data();
    for (int y = 0; y < r.height; ++y) {
        for (int x = 0; x < r.width; ++x) {
            uint8_t red=*px++, green=*px++, blue=*px++, alpha=*px++;
            if (alpha > 0) {
                uint32_t color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                GUI_SetColor(color);
                GUI_DrawPixel(x0 + x, y0 + y);
            }
        }
    }
    return;
#else
    // -- MCU streaming path (below) ------------------------------------

    // -- Parse picture header --------------------------------------------
    CSGPicture    pic;
    const uint8_t* compData   = nullptr;
    size_t         compSize   = 0;

    if (!ParseCsgPicture(csgData, pPic->Size, picIndex,
                         &pic, &compData, &compSize))
        return;

    // -- Allocate line buffer from GUIHEAP ------------------------------
    GUI_HMEM hLineBuf = GUI_ALLOC_AllocZero(CSG_LINEBUF_SIZE);
    if (hLineBuf == 0)
        return;
    uint8_t* lineBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hLineBuf));

    // -- Read embedded palette (if any) ---------------------------------
    const uint8_t* embeddedPalette = nullptr;
    int bpc = pic.BytesPerColor();
    int crn = pic.crn;

    if (crn > 0 && pic.ppos > 0 && pic.ppos + (crn * bpc) <= pic.size) {
        embeddedPalette = compData + pic.ppos - kCsgPictureHeaderSize;
    }

    const uint8_t* activePalette = (extPalette != nullptr)
                                   ? extPalette : embeddedPalette;

    // -- Init streaming decoder -----------------------------------------
    CSGDecoderState state;
    ColorMode outMode = static_cast<ColorMode>(pic.colorMode);

    CSG_ErrCode err = CsgDecodeInit(&state, &pic, lineBuf,
                                    activePalette, outMode);
    if (err != CSG_ErrCode::kOk) {
        GUI_ALLOC_Free(hLineBuf);
        return;
    }

    // MUST set stream AFTER Init (clears it to nullptr)
    state.stream = compData + (pic.dpos - kCsgPictureHeaderSize);

    // -- Decode and draw row by row -------------------------------------
    int width  = pic.width;
    int height = pic.height;
    int totalPixels = width * height;

    // CsgDecodePixels always outputs CRM data (bpc bytes per pixel).
    // The palette is already applied during decompression.
    int maxPixelsPerBatch = CSG_LINEBUF_SIZE / bpc;  // e.g. 1024/2 = 512 for RGB565

    while (state.pixelsDecoded < totalPixels) {
        int needed = totalPixels - state.pixelsDecoded;
        int batchSize = (needed > maxPixelsPerBatch) ? maxPixelsPerBatch : needed;

        int prevDecoded = state.pixelsDecoded;
        CsgDecodePixels(&state, lineBuf, batchSize);
        int actuallyDecoded = state.pixelsDecoded - prevDecoded;

        for (int i = 0; i < actuallyDecoded; ++i) {
            int px = prevDecoded + i;
            int row = px / width;
            int col = px % width;

            const uint8_t* crmPixel = lineBuf + (i * bpc);
            uint8_t red, green, blue, alpha;
            CrmPixelToRgba(crmPixel, static_cast<uint8_t>(outMode),
                           &red, &green, &blue, &alpha);

            DrawPixel(x0 + col, y0 + row, red, green, blue, alpha);
        }
    }

    // -- Free line buffer -----------------------------------------------
    GUI_ALLOC_Free(hLineBuf);
#endif  // __vmSIMULATOR__
}
