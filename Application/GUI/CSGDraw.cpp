//-----------------------------------------------------------------------------
/*
 File        : CSGDraw.cpp
 Version     : V1.01
 By          : Wey. Silver Grid

 Description : CSG image drawing -- MCU-safe implementation.
               Follows the LCDX_Bitmap_Draw pattern with dual MCU / simulator
               drawing paths.  Uses streaming decoder (zero heap vectors).

 Date        : 2026.06.24 (V1.00 — initial implementation)
              2026.06.25 (V1.01 — saturation helpers, atlas sub-picture, picIndex param)
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
    // Use bit-extraction formulas matching CSGCommon.h (FromRGB565 etc.)
    // — identical to full CSGDecoder's ConvertToRGBA output
    uint8_t r = 0, g = 0, b = 0, a = 255;
    switch (colorMode) {
    case 0x21: { // RGB565
        uint16_t v = src[0] | (src[1] << 8);
        r = static_cast<uint8_t>((v >> 8) & 0xF8);
        g = static_cast<uint8_t>((v >> 3) & 0xFC);
        b = static_cast<uint8_t>((v << 3) & 0xF8);
        break;
    }
    case 0x22: { // BGR565
        uint16_t v = src[0] | (src[1] << 8);
        b = static_cast<uint8_t>((v >> 8) & 0xF8);
        g = static_cast<uint8_t>((v >> 3) & 0xFC);
        r = static_cast<uint8_t>((v << 3) & 0xF8);
        break;
    }
    case 0x31: // RGB666
        r = static_cast<uint8_t>((src[0] & 0xFC) | ((src[2] << 2) & 0x03));
        g = static_cast<uint8_t>(((src[0] & 0x03) << 4) | ((src[1] >> 4) & 0x0F));
        b = static_cast<uint8_t>(((src[1] & 0x0F) << 2) | ((src[2] >> 6) & 0x03));
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
// Saturation helpers
//=============================================================================

/// Apply saturation to a single RGBA pixel (in-place).
/// sat: 0–100 (100 = no change, 0 = grayscale)
static inline void SaturatePixel(uint8_t* r, uint8_t* g, uint8_t* b, int sat)
{
    if (sat >= 100) return;
    if (sat < 0) sat = 0;

    // ITU-R BT.601 luma
    int gray = (static_cast<int>(*r) * 299 +
                static_cast<int>(*g) * 587 +
                static_cast<int>(*b) * 114) / 1000;

    int factor = sat * 10;  // 0..1000
    *r = static_cast<uint8_t>((gray * (1000 - factor) + static_cast<int>(*r) * factor) / 1000);
    *g = static_cast<uint8_t>((gray * (1000 - factor) + static_cast<int>(*g) * factor) / 1000);
    *b = static_cast<uint8_t>((gray * (1000 - factor) + static_cast<int>(*b) * factor) / 1000);
}

/// Apply saturation to a CRM-encoded palette.
/// palBytes: pointer to palette data, modified in-place.
/// entryCount: number of palette entries
/// bpc: bytes per color
/// colorMode: ColorMode enum value
/// sat: 0–100
static void SaturatePalette(uint8_t* palBytes, int entryCount, int bpc,
                            uint8_t colorMode, int sat)
{
    if (sat >= 100 || palBytes == nullptr || entryCount <= 0) return;
    if (sat < 0) sat = 0;

    for (int i = 0; i < entryCount; ++i) {
        uint8_t* entry = palBytes + i * bpc;
        uint8_t r = 0, g = 0, b = 0, a = 0;
        CrmPixelToRgba(entry, colorMode, &r, &g, &b, &a);
        SaturatePixel(&r, &g, &b, sat);

        // Pack back to CRM format
        switch (colorMode) {
        case 0x21: { // RGB565
            uint16_t v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            entry[0] = static_cast<uint8_t>(v & 0xFF);
            entry[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            break;
        }
        case 0x22: { // BGR565
            uint16_t v = ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3);
            entry[0] = static_cast<uint8_t>(v & 0xFF);
            entry[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            break;
        }
        case 0x31: // RGB666
            entry[0] = r * 63 / 255; entry[1] = g * 63 / 255; entry[2] = b * 63 / 255;
            break;
        case 0x33: // RGB888
            entry[0] = r; entry[1] = g; entry[2] = b;
            break;
        case 0x41: // RGB8888
            entry[0] = r; entry[1] = g; entry[2] = b; entry[3] = a;
            break;
        default: break;
        }
    }
}

//=============================================================================
// Unified CSG drawing (streaming decoder, Sim + MCU single path)
//=============================================================================

/// Batch size in pixels — balance between GUI_ALLOC footprint and call count
constexpr int kCsgBatchPixels = 128;

void CSG_DrawPicture(const TGUIPicture* pPic, int x0, int y0,
                     int picIndex, int saturation)
{
    if (pPic == nullptr || pPic->pData == nullptr) return;
    if (pPic->Type != ID_CSG) return;

    if (saturation < 10) saturation = 10;
    if (saturation > 100) saturation = 100;

    const uint8_t* csgData = static_cast<const uint8_t*>(pPic->pData);

    // 1. Parse picture header (handles atlas sub-pictures)
    CSGPicture    pic;
    const uint8_t* compData = nullptr;
    size_t         compSize = 0;

    if (!ParseCsgPicture(csgData, pPic->Size, picIndex,
                         &pic, &compData, &compSize))
        return;

    int bpc = pic.BytesPerColor();
    int crn = pic.crn;
    int totalPixels = pic.width * pic.height;
    ColorMode outMode = static_cast<ColorMode>(pic.colorMode);

    // 2. Read embedded palette
    const uint8_t* embPalette = nullptr;
    if (crn > 0 && pic.ppos > 0 &&
        pic.ppos + static_cast<uint16_t>(crn * bpc) <= pic.size) {
        embPalette = compData + pic.ppos - kCsgPictureHeaderSize;
    }

    // 3. Apply saturation to palette
    uint8_t* satPalBuf = nullptr;
    GUI_HMEM hPalBuf = 0;
    const uint8_t* activePalette = embPalette;
    if (saturation != 100 && embPalette != nullptr && crn > 0) {
        int palBytes = crn * bpc;
        hPalBuf = GUI_ALLOC_AllocZero(palBytes);
        if (hPalBuf != 0) {
            satPalBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hPalBuf));
            memcpy(satPalBuf, embPalette, palBytes);
            SaturatePalette(satPalBuf, crn, bpc, pic.colorMode, saturation);
            activePalette = satPalBuf;
        }
    }

    // 4. Allocate CRM output buffer — one row (minimal)
    int maxBatch = pic.width;
    if (maxBatch > totalPixels) maxBatch = totalPixels;
    int bufBytes = maxBatch * bpc;
    GUI_HMEM hOutBuf = GUI_ALLOC_AllocZero(bufBytes);
    if (hOutBuf == 0) {
        if (hPalBuf != 0) GUI_ALLOC_Free(hPalBuf);
        return;
    }
    uint8_t* outBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hOutBuf));

    // 5. Init streaming decoder (unified Sim+MCU path, CAS 0/1/2/3)
    CSGDecoderState state;
    if (CsgDecodeInit(&state, &pic, outBuf, activePalette, outMode)
        != CSG_ErrCode::kOk) {
        GUI_ALLOC_Free(hOutBuf);
        if (hPalBuf != 0) GUI_ALLOC_Free(hPalBuf);
        return;
    }
    state.stream     = compData + (pic.dpos - kCsgPictureHeaderSize);
    state.streamSize = compSize - (pic.dpos - kCsgPictureHeaderSize);

    int width = pic.width;
    while (state.pixelsDecoded < totalPixels) {
        int prevDecoded = state.pixelsDecoded;
        CSG_ErrCode decErr = CsgDecodePixels(&state, outBuf, maxBatch);
        int n = state.pixelsDecoded - prevDecoded;
        if (decErr != CSG_ErrCode::kOk || n == 0) break;  // error or stall
        for (int i = 0; i < n; ++i) {
            int pxIdx = prevDecoded + i;
            int row   = pxIdx / width;
            int col   = pxIdx % width;
            const uint8_t* crm = outBuf + i * bpc;

            // Transparency: CRN>0 → CRI=0 maps to palette[0], skip
            if (crn > 0 && activePalette != nullptr) {
                bool isTransp = true;
                for (int k = 0; k < bpc; ++k) {
                    if (crm[k] != activePalette[k]) { isTransp = false; break; }
                }
                if (isTransp) continue;
            }

            uint8_t red, green, blue, alpha;
            CrmPixelToRgba(crm, pic.colorMode, &red, &green, &blue, &alpha);
            if (saturation != 100 && crn == 0)
                SaturatePixel(&red, &green, &blue, saturation);
            DrawPixel(x0 + col, y0 + row, red, green, blue, alpha);
        }
    }

    // Free DEFLATE intermediate buffer if allocated (GUI_ALLOC, works Sim+MCU)
    if (state.deflateBufHandle != 0) {
        GUI_ALLOC_Free(static_cast<GUI_HMEM>(state.deflateBufHandle));
    }

    GUI_ALLOC_Free(hOutBuf);
    if (hPalBuf != 0) GUI_ALLOC_Free(hPalBuf);
}
