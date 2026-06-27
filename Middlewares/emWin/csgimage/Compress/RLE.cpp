//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : RLE.cpp
 Version     : V1.51
 By          : Wey. Silver Grid

 Description : RLE codec implementation — ZRC/DPS/CPS/CPL encode/decode,
               DPS cross-batch pending, streaming decoder support.

 Date        : 2026.06.26 (V1.51 — DPS cross-batch pending in streaming decoder)
              2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#include "Compress/RLE.h"
#include <cstring>
#include <algorithm>

constexpr uint32_t RLE_FRAME_ZRC = 0b00;
constexpr uint32_t RLE_FRAME_DPS = 0b01;
constexpr uint32_t RLE_FRAME_CPS = 0b10;
constexpr uint32_t RLE_FRAME_CPL = 0b11;


// ============================================================================
// Helpers
// ============================================================================

void CompressionBase::SetCal(uint8_t cal) {
    cal_ = (cal > 7) ? 2 : cal;
}

bool RleCodec::IsTransparent(const uint8_t* pixel, int crn, ColorMode cm,
                              const uint8_t* transparentVal) const {
    int bpc = BytesPerColor(cm);
    if (crn > 0) {
        // CRN > 0: pixel is a palette index (stored as uint8_t)
        return *pixel == 0;
    } else {
        // CRN == 0: pixel is CRM-encoded; compare with palette[0]
        return std::memcmp(pixel, transparentVal, bpc) == 0;
    }
}

uint8_t RleCodec::TransparentIndex(int crn) const {
    (void)crn;
    return 0;  // CRI=0 is transparent when CRN>0
}

// ============================================================================
// Encoder — frame writers
// ============================================================================

void RleCodec::WriteZRC(BitPacker& bp, int count) {
    // count ∈ [1, 63]
    uint8_t header = static_cast<uint8_t>(((count & kRleCountMask6) << kRleCountShift) | RLE_FRAME_ZRC);
    bp.PackBits(header, 8);
}

void RleCodec::WriteDPS(BitPacker& bp, const uint8_t* pixels, int count,
                         int crn, ColorMode cm) {
    // count ∈ [1, 64], N = count - 1
    int N = count - 1;
    uint8_t header = static_cast<uint8_t>(((N & kRleCountMask6) << kRleCountShift) | RLE_FRAME_DPS);
    bp.PackBits(header, 8);

    if (crn > 0) {
        int bpi = BitsPerIndex(crn);
        for (int i = 0; i < count; ++i)
            bp.PackBits(pixels[i], bpi);
    } else {
        int bpc = BytesPerColor(cm);
        for (int i = 0; i < count; ++i) {
            const uint8_t* p = pixels + i * bpc;
            for (int j = 0; j < bpc; ++j)
                bp.PackBits(p[j], 8);
        }
    }
}

void RleCodec::WriteCPS(BitPacker& bp, const uint8_t* pixel, int count,
                         int crn, ColorMode cm) {
    // count ∈ [1, 64], N = count - 1
    int N = count - 1;
    uint8_t header = static_cast<uint8_t>(((N & kRleCountMask6) << kRleCountShift) | RLE_FRAME_CPS);
    bp.PackBits(header, 8);

    if (crn > 0) {
        int bpi = BitsPerIndex(crn);
        bp.PackBits(*pixel, bpi);
    } else {
        int bpc = BytesPerColor(cm);
        for (int j = 0; j < bpc; ++j)
            bp.PackBits(pixel[j], 8);
    }
}

void RleCodec::WriteCPL(BitPacker& bp, const uint8_t* pixel, int count,
                         int crn, ColorMode cm) {
    // count ∈ [1, 16384], N = count - 1, N stored as 14 bits
    int N = count - 1;
    uint8_t low = static_cast<uint8_t>((N & kRleCountMask6) << kRleCountShift) | RLE_FRAME_CPL;
    uint8_t high = static_cast<uint8_t>((N >> kRleCplShift) & kByteMask);
    bp.PackBits(low, 8);
    bp.PackBits(high, 8);

    if (crn > 0) {
        int bpi = BitsPerIndex(crn);
        bp.PackBits(*pixel, bpi);
    } else {
        int bpc = BytesPerColor(cm);
        for (int j = 0; j < bpc; ++j)
            bp.PackBits(pixel[j], 8);
    }
}

// ============================================================================
// Compress (Encoder)
// ============================================================================

std::vector<uint8_t> RleCodec::Compress(
    const uint8_t* pixels,
    int width,
    int height,
    int crn,
    ColorMode cm) {

    BitPacker bp;
    int totalPixels = width * height;
    int bpc = BytesPerColor(cm);
    int pixelStride = (crn > 0) ? 1 : bpc;

    // Transparent color marker for CRN==0
    // In CRN=0 mode, the first palette entry is the transparent color.
    // The encoder treats a pixel as transparent when it matches palette[0].
    // Since the encoder receives raw CRM data, we use a zero sentinel for CRN>0.

    int pos = 0;
    while (pos < totalPixels) {
        const uint8_t* cur = pixels + pos * pixelStride;
        int remaining = totalPixels - pos;

        // ---- Strategy: prefer longest runs first ----

        // 1. Check for transparent run (ZRC)
        // Transparent is CRI=0 for CRN>0, or matches transparent for CRN=0
        if (crn > 0 && *cur == 0) {
            int runLen = 0;
            while (pos + runLen < totalPixels
                   && pixels[(pos + runLen) * pixelStride] == 0
                   && runLen < kRleZrcMax) {
                ++runLen;
            }
            WriteZRC(bp, runLen);
            pos += runLen;
            continue;
        }

        // 2. Check for continuous same-value run
        int runLen = 1;
        while (pos + runLen < totalPixels && runLen < kRleCplMax) {
            const uint8_t* next = pixels + (pos + runLen) * pixelStride;
            bool same = (pixelStride == 1)
                ? (*next == *cur)
                : (std::memcmp(next, cur, pixelStride) == 0);
            if (!same) break;
            ++runLen;
        }

        if (runLen >= kRleMinRun) {
            // Use CPS for short runs, CPL for longer
            if (runLen <= kRleCpsMax) {
                WriteCPS(bp, cur, runLen, crn, cm);
            } else {
                WriteCPL(bp, cur, runLen, crn, cm);
            }
            pos += runLen;
            continue;
        }

        // 3. Discrete pixels (DPS) — collect up to 64
        int dpsLen = 0;
        while (pos + dpsLen < totalPixels && dpsLen < kRleDpsMax) {
            // Look ahead: if next pixel starts a run of ≥3, stop DPS
            if (dpsLen > 0 && pos + dpsLen + 2 < totalPixels) {
                const uint8_t* ahead = pixels + (pos + dpsLen) * pixelStride;
                const uint8_t* ahead2 = pixels + (pos + dpsLen + 1) * pixelStride;
                const uint8_t* ahead3 = pixels + (pos + dpsLen + 2) * pixelStride;

                bool isRun = (pixelStride == 1)
                    ? (*ahead == *ahead2 && *ahead == *ahead3)
                    : (std::memcmp(ahead, ahead2, pixelStride) == 0
                       && std::memcmp(ahead, ahead3, pixelStride) == 0);

                // Also stop if transparent (for CRN>0)
                if (crn > 0 && *ahead == 0)
                    break;
                if (isRun)
                    break;
            }
            ++dpsLen;
        }
        WriteDPS(bp, cur, dpsLen, crn, cm);
        pos += dpsLen;
    }

    return bp.GetBytes();
}

// ============================================================================
// Decompress (Decoder)
// ============================================================================

CSG_ErrCode RleCodec::Decompress(
    const uint8_t* input,
    size_t inputLen,
    uint8_t* output,
    int width,
    int height,
    int crn,
    ColorMode cm) {

    int totalPixels = width * height;
    int bpc = BytesPerColor(cm);
    int bpi = (crn > 0) ? BitsPerIndex(crn) : 0;
    int pixelStride = (crn > 0) ? 1 : bpc;
    int outPos = 0;

    BitUnpacker bu(input, inputLen);

    while (outPos < totalPixels) {
        if (bu.BitsRemaining() < 8) {
            // Allow trailing zero-padding; if we've produced all pixels, done
            if (outPos == totalPixels) break;
            return CSG_ErrCode::kErrRleFrameErr;
        }

        uint8_t header = bu.UnpackBits(8);
        uint8_t frameType = header & kRleFrameTypeMask;
        uint8_t N_short   = (header >> kRleCountShift) & kRleCountMask6;

        switch (frameType) {
        case RLE_FRAME_ZRC: { // ZRC — transparent run
            if (N_short == 0) return CSG_ErrCode::kErrRleFrameErr;
            int count = N_short;  // 1..63
            if (outPos + count > totalPixels)
                return CSG_ErrCode::kErrRleFrameErr;

            for (int i = 0; i < count; ++i) {
                if (crn > 0) {
                    output[outPos * pixelStride] = 0;  // CRI=0 transparent
                } else {
                    // All bytes zero (transparent CRM representation)
                    std::memset(output + outPos * pixelStride, 0, bpc);
                }
                ++outPos;
            }
            break;
        }

        case RLE_FRAME_DPS: { // DPS — discrete pixels
            int count = N_short + 1;  // 1..64
            if (outPos + count > totalPixels)
                return CSG_ErrCode::kErrRleFrameErr;

            for (int i = 0; i < count; ++i) {
                if (crn > 0) {
                    if (static_cast<int>(bu.BitsRemaining()) < bpi)
                        return CSG_ErrCode::kErrRleCriMiss;
                    output[outPos * pixelStride] = bu.UnpackBits(bpi);
                } else {
                    for (int j = 0; j < bpc; ++j) {
                        if (bu.BitsRemaining() < 8)
                            return CSG_ErrCode::kErrRleCriMiss;
                        output[outPos * pixelStride + j] = bu.UnpackBits(8);
                    }
                }
                ++outPos;
            }
            break;
        }

        case RLE_FRAME_CPS: { // CPS — short continuous run
            int count = N_short + 1;  // 1..64
            if (outPos + count > totalPixels)
                return CSG_ErrCode::kErrRleFrameErr;

            uint8_t colorVal[4] = {0};
            if (crn > 0) {
                if (static_cast<int>(bu.BitsRemaining()) < bpi)
                    return CSG_ErrCode::kErrRleCriMiss;
                colorVal[0] = bu.UnpackBits(bpi);
            } else {
                for (int j = 0; j < bpc; ++j) {
                    if (bu.BitsRemaining() < 8)
                        return CSG_ErrCode::kErrRleCriMiss;
                    colorVal[j] = bu.UnpackBits(8);
                }
            }

            for (int i = 0; i < count; ++i) {
                std::memcpy(output + outPos * pixelStride, colorVal, pixelStride);
                ++outPos;
            }
            break;
        }

        case RLE_FRAME_CPL: { // CPL — long continuous run
            if (bu.BitsRemaining() < 8)
                return CSG_ErrCode::kErrRleCplTrunc;
            uint8_t N_high = bu.UnpackBits(8);
            int N = N_short | (static_cast<int>(N_high) << 6);
            int count = N + 1;  // 1..16384
            if (outPos + count > totalPixels)
                return CSG_ErrCode::kErrRleFrameErr;

            uint8_t colorVal[4] = {0};
            if (crn > 0) {
                if (static_cast<int>(bu.BitsRemaining()) < bpi)
                    return CSG_ErrCode::kErrRleCriMiss;
                colorVal[0] = bu.UnpackBits(bpi);
            } else {
                for (int j = 0; j < bpc; ++j) {
                    if (bu.BitsRemaining() < 8)
                        return CSG_ErrCode::kErrRleCriMiss;
                    colorVal[j] = bu.UnpackBits(8);
                }
            }

            for (int i = 0; i < count; ++i) {
                std::memcpy(output + outPos * pixelStride, colorVal, pixelStride);
                ++outPos;
            }
            break;
        }
        }  // end switch
    }  // end while

    return (outPos == totalPixels)
        ? CSG_ErrCode::kOk
        : CSG_ErrCode::kErrRleFrameErr;
}
