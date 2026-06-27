//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CSGDecoder.cpp
 Version     : V1.52
 By          : Wey. Silver Grid

 Description : CSG streaming decoder (MCU-safe).  CAS 0/1/2/3 dispatch.
               MiniLZ77 circular window, RLE cross-batch DPS/ZRC/CPS/CPL pending,
               DEFLATE Huffman→MiniLZ77 pipeline, GUI_ALLOC buffer management.

 Date        : 2026.06.26 (V1.52 — DPS cross-batch pending fix, origCount guard
                          raised to kCsgMaxWidth*kCsgMaxHeight)
              2026.06.26 (V1.51 — streaming CAS 0/1/2/3, RLE pendRle, MiniLZ77
                          pendMatchLen/pendRef, circular window indexing)
              2026.06.25 (V1.50 — original CSG v1.5 decoder)
*/
//-----------------------------------------------------------------------------
#include "CSGDecoder.h"
#include "Compress/RLE.h"
#include "Compress/Huffman.h"
#include "Compress/MiniLZ77.h"
#include <cstring>
#include <algorithm>
#include "CSGCommon.h"
#include "GUI.h"  // GUI_ALLOC_AllocZero / GUI_ALLOC_Free for DEFLATE buffer

// ============================================================================
// Construction
// ============================================================================

CSGDecoder::CSGDecoder() = default;

// ============================================================================
// DecompressData — decompress picture data to raw pixel bytes
// ============================================================================

CSG_ErrCode CSGDecoder::DecompressData(const uint8_t* compData,
                                        size_t compLen,
                                        const CSGPicture& header,
                                        std::vector<uint8_t>& rawOutput) {
    auto algo = header.GetCompressAlgorithm();
    int cal  = header.GetCompressLevel();
    auto cm  = static_cast<ColorMode>(header.colorMode);
    int crn  = header.crn;
    int pixelCount = header.width * header.height;
    int bpc  = BytesPerColor(cm);
    int bpi  = (crn > 0) ? BitsPerIndex(crn) : 0;

    int rawStride = (crn > 0) ? 1 : bpc;
    int expectedRawSize = pixelCount * rawStride;
    rawOutput.resize(expectedRawSize);

    if (compLen == 0)
        return CSG_ErrCode::kErrCompStreamEmpty;

    if (algo == CompressAlgorithm::kNone) {
        // No compression — raw bit-packed (CRN>0) or CRM bytes (CRN=0)
        if (crn > 0) {
            // Bit-unpack CRI indices
            BitUnpacker bu(compData, compLen);
            for (int i = 0; i < pixelCount; ++i) {
                if (bu.BitsRemaining() < static_cast<size_t>(bpi))
                    return CSG_ErrCode::kErrBitpackTrunc;
                rawOutput[i] = bu.UnpackBits(bpi);
            }
        } else {
            // Raw CRM bytes
            if (compLen < static_cast<size_t>(expectedRawSize))
                return CSG_ErrCode::kErrBitpackTrunc;
            std::memcpy(rawOutput.data(), compData, expectedRawSize);
        }
        return CSG_ErrCode::kOk;
    }

    if (algo == CompressAlgorithm::kDEFLATE) {
        // DEFLATE = MiniLZ77 output bytes → Huffman (VP-style)
        auto lzBytes = HuffmanCodec::DecompressRaw(compData, compLen);
        if (lzBytes.empty()) return CSG_ErrCode::kErrCompStreamEmpty;

        MiniLZ77Codec lz77;
        return lz77.Decompress(lzBytes.data(), lzBytes.size(),
                               rawOutput.data(), header.width, header.height,
                               header.crn, static_cast<ColorMode>(header.colorMode));
    }

    // Standalone codecs (RLE, Huffman, MiniLZ77)
    auto codec = CreateCodec(algo, static_cast<uint8_t>(cal));
    if (!codec)
        return CSG_ErrCode::kErrCasInvalid;

    return codec->Decompress(compData, compLen, rawOutput.data(),
                              header.width, header.height,
                              crn, cm);
}

// ============================================================================
// ConvertToRGBA — raw CRM/CRI + palette → RGBA pixel array
// ============================================================================

CSG_ErrCode CSGDecoder::ConvertToRGBA(
    const uint8_t* rawData,
    const CSGPicture& header,
    const std::vector<uint8_t>& palette,
    std::vector<uint8_t>& rgbaOutput) {

    auto cm = static_cast<ColorMode>(header.colorMode);
    int crn = header.crn;
    int pixelCount = header.width * header.height;
    int bpc = BytesPerColor(cm);

    rgbaOutput.resize(pixelCount * 4);

    for (int i = 0; i < pixelCount; ++i) {
        VxARGB px;
        bool isTransparent = false;

        if (crn > 0) {
            // Palette mode: rawData[i] is a CRI palette index
            uint8_t cri = rawData[i];
            if (cri == 0) {
                // CRI=0 is transparent
                isTransparent = true;
            }
            // Convert CRI to palette entry
            int palOffset = cri * bpc;
            if (palOffset + bpc <= static_cast<int>(palette.size())) {
                px = UnpackColor(palette.data() + palOffset, cm);
            }
        } else {
            // True-color mode: rawData[i*bpc] is CRM-encoded
            int offset = i * bpc;
            px = UnpackColor(rawData + offset, cm);

            // Transparent if pixel matches palette[0]
            if (palette.size() >= static_cast<size_t>(bpc)) {
                VxARGB transColor = UnpackColor(palette.data(), cm);
                if (px == transColor)
                    isTransparent = true;
            }
        }

        // Write RGBA
        uint8_t* out = rgbaOutput.data() + i * 4;
        out[0] = px.r;
        out[1] = px.g;
        out[2] = px.b;
        out[3] = isTransparent ? 0 : px.a;
    }

    return CSG_ErrCode::kOk;
}

// ============================================================================
// DecodePicture — decode a single CSGPicture from encoded bytes
// ============================================================================

DecoderResult CSGDecoder::DecodePicture(const uint8_t* data, size_t len) {
    DecoderResult result;

    // Parse picture header
    CSGPicture header;
    CSG_ErrCode err = DecodePictureHeader(data, len, header);
    if (err != CSG_ErrCode::kOk) {
        result.error = err;
        result.errorDetail = CsgGetErrStr(err);
        return result;
    }

    result.width     = header.width;
    result.height    = header.height;
    result.colorMode = static_cast<ColorMode>(header.colorMode);
    result.crn       = header.crn;
    result.algo      = header.GetCompressAlgorithm();
    result.cal       = header.GetCompressLevel();

    int bpc = BytesPerColor(static_cast<ColorMode>(header.colorMode));
    int palBytes = (header.crn > 0)
        ? header.crn * bpc
        : bpc;

    // Read palette
    uint32_t ppos = header.ppos;
    if (ppos + palBytes > len) {
        result.error = CSG_ErrCode::kErrPaletteRead;
        result.errorDetail = "Palette data beyond buffer";
        return result;
    }
    result.palette.assign(data + ppos, data + ppos + palBytes);

    // Decompress pixel data
    uint32_t dpos = header.dpos;
    size_t compLen = header.size - dpos;
    if (dpos + compLen > len) {
        result.error = CSG_ErrCode::kErrDposOverflow;
        result.errorDetail = "Pixel data beyond buffer";
        return result;
    }

    std::vector<uint8_t> rawData;
    err = DecompressData(data + dpos, compLen, header, rawData);
    if (err != CSG_ErrCode::kOk) {
        result.error = err;
        result.errorDetail = CsgGetErrStr(err);
        return result;
    }

    // Convert to RGBA
    err = ConvertToRGBA(rawData.data(), header, result.palette, result.pixels);
    if (err != CSG_ErrCode::kOk) {
        result.error = err;
        result.errorDetail = CsgGetErrStr(err);
        return result;
    }

    result.error = CSG_ErrCode::kOk;
    return result;
}

// ============================================================================
// DecodeAtlasPicture — decode one picture from an atlas
// ============================================================================

DecoderResult CSGDecoder::DecodeAtlasPicture(const uint8_t* atlasData,
                                              size_t atlasLen,
                                              int pictureIndex) {
    DecoderResult result;

    CSGHeader header;
    CSG_ErrCode err = DecodeAtlasHeader(atlasData, atlasLen, header);
    if (err != CSG_ErrCode::kOk) {
        result.error = err;
        result.errorDetail = CsgGetErrStr(err);
        return result;
    }

    if (pictureIndex < 0
        || pictureIndex >= static_cast<int>(header.GetPicCount())) {
        result.error = CSG_ErrCode::kErrOutOfRange;
        result.errorDetail = "Picture index out of range";
        return result;
    }

    std::vector<uint32_t> offsets = ReadPicOffsets(atlasData, atlasLen);
    if (offsets.empty() || static_cast<size_t>(pictureIndex) >= offsets.size()) {
        result.error = CSG_ErrCode::kErrOffsetTableBroken;
        result.errorDetail = "Failed to read picture offset";
        return result;
    }
    uint32_t offset = offsets[pictureIndex];
    if (offset >= atlasLen) {
        result.error = CSG_ErrCode::kErrOffsetTableBroken;
        result.errorDetail = "Offset beyond file bounds";
        return result;
    }

    return DecodePicture(atlasData + offset, atlasLen - offset);
}

// ============================================================================
// DecodeAtlas — decode all pictures from an atlas
// ============================================================================

std::vector<DecoderResult> CSGDecoder::DecodeAtlas(const uint8_t* atlasData,
                                                    size_t atlasLen) {
    std::vector<DecoderResult> results;

    CSGHeader header;
    CSG_ErrCode err = DecodeAtlasHeader(atlasData, atlasLen, header);
    if (err != CSG_ErrCode::kOk) {
        DecoderResult r;
        r.error = err;
        r.errorDetail = CsgGetErrStr(err);
        results.push_back(r);
        return results;
    }

    std::vector<uint32_t> offsets = ReadPicOffsets(atlasData, atlasLen);
    if (offsets.empty()) {
        DecoderResult r;
        r.error = CSG_ErrCode::kErrOffsetTableBroken;
        r.errorDetail = "Failed to read picture offsets";
        results.push_back(r);
        return results;
    }

    int picCount = static_cast<int>(offsets.size());
    for (int i = 0; i < picCount; ++i) {
        uint32_t offset = offsets[i];
        if (offset >= atlasLen) {
            DecoderResult r;
            r.error = CSG_ErrCode::kErrOffsetTableBroken;
            r.errorDetail = "Offset " + std::to_string(offset)
                          + " beyond bounds";
            results.push_back(r);
            continue;
        }
        results.push_back(DecodePicture(atlasData + offset,
                                        atlasLen - offset));
    }

    return results;
}

// ============================================================================
// Streaming decoder API (for MCU use)
// ============================================================================

// Streaming decoder API (for MCU use)
// ============================================================================
// Usage:
//   CSGDecoderState st;
//   csgDecodeInit(&st, &pic, lineBuf);
//   st.stream = compressedData;   // point to compressed data in Flash
//   st.window = historyBuf;       // point to output-history buffer
//   st.windowSize = sizeof(historyBuf);  // must be >= image total pixels
//   while (st.pixelsDecoded < total) {
//       csgDecodePixels(&st, out, 32);   // decode up to 32 pixels at a time
//   }

CSG_ErrCode CsgDecodeInit(CSGDecoderState* state,
                           const CSGPicture* pic,
                           uint8_t* lineBuf,
                           const uint8_t* palette,
                           ColorMode outMode) {
    if (!state || !pic || !lineBuf)
        return CSG_ErrCode::kErrSrcImgNull;

    state->stream     = nullptr;
    state->streamSize = 0;
    state->lineBuf    = lineBuf;
    state->width   = pic->width;
    state->height  = pic->height;
    state->cal     = pic->GetCompressLevel();
    state->cas     = static_cast<int>(pic->GetCompressAlgorithm());
    state->pixelsDecoded = 0;
    state->bitBuf   = 0;
    state->bitCount = 8;
    state->linePos  = 0;
    state->rowCount = 0;
    state->windowPos = 0;
    state->windowSize = 0;   // reset — only DEFLATE path sets this
    state->pendMatchLen = 0; // reset — no pending match
    state->pendRef      = 0;
    state->pendRleCount = 0; // reset — no pending RLE frame
    state->pendRleType  = -1;
    state->pendRleCri   = 0;
    state->deflateBufHandle = 0;  // reset — no DEFLATE buffer allocated

    state->bpc     = BytesPerColor(outMode);
    state->palette = palette;
    state->crn     = pic->crn;
    state->bpi     = (pic->crn > 0) ? BitsPerIndex(pic->crn) : 0;

    // Store transparent color
    if (palette && state->bpc <= 4) {
        for (int i = 0; i < state->bpc; ++i)
            state->transparent[i] = palette[i];
    } else {
        state->transparent[0] = state->transparent[1] = 0;
        state->transparent[2] = state->transparent[3] = 0;
    }

    return CSG_ErrCode::kOk;
}

//=============================================================================
// Internal: write one CRM pixel to output, applying palette lookup if CRN>0
//=============================================================================
static inline void WriteCrmPixel(uint8_t* output, int idx, int bpc,
                                  const uint8_t* pal, uint8_t cri) {
    if (pal && bpc > 0) {
        for (int k = 0; k < bpc; ++k)
            output[idx * bpc + k] = pal[cri * bpc + k];
    } else {
        output[idx] = cri;
    }
}

//=============================================================================
// CAS=3: MiniLZ77 streaming decoder
//=============================================================================
static CSG_ErrCode DecodeMiniLZ77(CSGDecoderState* st,
                                   uint8_t* output, int toDecode) {
    const uint8_t* input = st->stream;
    int     ip      = st->windowPos;
    uint8_t ctrl    = static_cast<uint8_t>(st->bitBuf);
    int     tokenIdx = st->bitCount;
    int     outPos  = st->pixelsDecoded;
    uint8_t* win    = st->window;
    int     bpc     = st->bpc;
    const uint8_t* pal = st->palette;
    int     dec     = 0;

    // Resume pending match from previous batch
    if (st->pendMatchLen > 0) {
        int ref = st->pendRef;
        for (int j = 0; j < st->pendMatchLen && dec < toDecode; ++j) {
            uint8_t cri = win[(ref + j) % CSG_DEC_WINDOW_BYTES];
            win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
            WriteCrmPixel(output, dec, bpc, pal, cri);
            ++outPos; ++dec;
        }
        int consumed = dec;
        st->pendMatchLen -= consumed;
        st->pendRef += consumed;
    }

    while (dec < toDecode) {
        if (tokenIdx >= 8) {
            ctrl     = input[ip++];
            tokenIdx = 0;
        }
        uint8_t cri;
        if (ctrl & (1u << tokenIdx)) {
            int encLen   = input[ip++];
            int encDist  = input[ip++];
            int matchLen = encLen + 3;
            int dist     = encDist + 1;
            if (dist > outPos) return CSG_ErrCode::kErrLz77DistErr;
            int ref = outPos - dist;
            int j = 0;
            for (; j < matchLen && dec < toDecode; ++j) {
                cri = win[(ref + j) % CSG_DEC_WINDOW_BYTES];
                win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
            if (j < matchLen) {
                st->pendMatchLen = matchLen - j;
                st->pendRef     = ref + j;
            }
        } else {
            cri = input[ip++];
            win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
            WriteCrmPixel(output, dec, bpc, pal, cri);
            ++outPos; ++dec;
        }
        ++tokenIdx;
    }

    st->pixelsDecoded = outPos;
    st->bitBuf   = ctrl;
    st->bitCount = tokenIdx;
    st->windowPos = ip;
    return CSG_ErrCode::kOk;
}

//=============================================================================
// CAS=0: No compression — bit-unpack or direct copy
//=============================================================================
static CSG_ErrCode DecodeNone(CSGDecoderState* st,
                               uint8_t* output, int toDecode) {
    const uint8_t* input = st->stream;
    int ip   = st->windowPos;
    int outPos = st->pixelsDecoded;
    int bpc = st->bpc;
    int bpi = st->bpi;
    const uint8_t* pal = st->palette;
    int dec = 0;

    if (bpi > 0) {
        // Bit-unpack CRI indices
        uint32_t buf = static_cast<uint32_t>(st->bitBuf);
        int bits     = st->bitCount;
        while (dec < toDecode) {
            while (bits < bpi) {
                buf |= static_cast<uint32_t>(input[ip++]) << bits;
                bits += 8;
            }
            uint8_t cri = static_cast<uint8_t>(buf & ((1u << bpi) - 1));
            buf >>= bpi; bits -= bpi;
            WriteCrmPixel(output, dec, bpc, pal, cri);
            ++outPos; ++dec;
        }
        st->bitBuf = buf;
        st->bitCount = bits;
    } else {
        // Direct CRM copy
        int byteCount = toDecode * bpc;
        for (int i = 0; i < byteCount; ++i)
            output[i] = input[ip++];
        dec = toDecode;
        outPos += toDecode;
    }

    st->pixelsDecoded = outPos;
    st->windowPos = ip;
    return CSG_ErrCode::kOk;
}

//=============================================================================
// CAS=1: RLE streaming decoder
//=============================================================================
static CSG_ErrCode DecodeRLE(CSGDecoderState* st,
                              uint8_t* output, int toDecode) {
    const uint8_t* input = st->stream;
    int ip   = st->windowPos;
    int outPos = st->pixelsDecoded;
    int bpc = st->bpc;
    int bpi = st->bpi;
    const uint8_t* pal = st->palette;
    uint32_t buf = static_cast<uint32_t>(st->bitBuf);
    int bits     = st->bitCount;
    int dec = 0;

    auto refill = [&](int n) {
        while (bits < n) { buf |= static_cast<uint32_t>(input[ip++]) << bits; bits += 8; }
    };

    // Resume pending RLE frame from previous batch
    if (st->pendRleType >= 0 && st->pendRleCount > 0) {
        if (st->pendRleType == 1) {
            // DPS resume — each pixel reads its own CRI from the saved bit/byte position
            int n = st->pendRleCount;
            if (n > toDecode) n = toDecode;
            for (int j = 0; j < n; ++j) {
                uint8_t cri;
                if (bpi > 0) {
                    refill(bpi);
                    cri = static_cast<uint8_t>(buf & ((1u << bpi) - 1));
                    buf >>= bpi; bits -= bpi;
                } else {
                    cri = input[ip++];
                }
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
            st->pendRleCount -= n;
            if (st->pendRleCount == 0) st->pendRleType = -1;
        } else {
            // ZRC/CPS/CPL resume — all pixels share the same CRI
            int n = st->pendRleCount;
            if (n > toDecode) n = toDecode;
            uint8_t cri = (st->pendRleType == 0) ? 0 : st->pendRleCri;
            for (int j = 0; j < n; ++j) {
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
            st->pendRleCount -= n;
            if (st->pendRleCount == 0) st->pendRleType = -1;
        }
    }

    while (dec < toDecode) {
        refill(8);
        uint8_t header = static_cast<uint8_t>(buf & 0xFF);
        buf >>= 8; bits -= 8;
        int frameType = header & 0x03;
        int count     = (header >> 2) & 0x3F;

        if (frameType == 0) {  // ZRC — transparent run (1..63 pixels)
            int n = count;
            if (n > (toDecode - dec)) {
                st->pendRleCount = n - (toDecode - dec);
                st->pendRleType  = 0;
                n = toDecode - dec;
            }
            for (int j = 0; j < n; ++j) {
                WriteCrmPixel(output, dec, bpc, pal, 0);
                ++outPos; ++dec;
            }
        } else if (frameType == 1) {  // DPS — discrete pixels (1..64)
            int n = count + 1;
            int j = 0;
            for (; j < n && dec < toDecode; ++j) {
                uint8_t cri;
                if (bpi > 0) {
                    refill(bpi);
                    cri = static_cast<uint8_t>(buf & ((1u << bpi) - 1));
                    buf >>= bpi; bits -= bpi;
                } else {
                    cri = input[ip++];
                }
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
            if (j < n) {
                st->pendRleCount = n - j;
                st->pendRleType  = 1;  // DPS — resume reads individual CRIs
            }
        } else if (frameType == 2) {  // CPS — short continuous run (1..64)
            int n = count + 1;
            uint8_t cri;
            if (bpi > 0) {
                refill(bpi);
                cri = static_cast<uint8_t>(buf & ((1u << bpi) - 1));
                buf >>= bpi; bits -= bpi;
            } else {
                cri = input[ip++];
            }
            if (n > (toDecode - dec)) {
                st->pendRleCount = n - (toDecode - dec);
                st->pendRleType  = 2;
                st->pendRleCri   = cri;
                n = toDecode - dec;
            }
            for (int j = 0; j < n; ++j) {
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
        } else {  // CPL — long continuous run (1..16384)
            refill(8);
            int high = static_cast<int>(buf & 0xFF);
            buf >>= 8; bits -= 8;
            int cplN = ((high << 6) | count) + 1;
            uint8_t cri;
            if (bpi > 0) {
                refill(bpi);
                cri = static_cast<uint8_t>(buf & ((1u << bpi) - 1));
                buf >>= bpi; bits -= bpi;
            } else {
                cri = input[ip++];
            }
            if (cplN > (toDecode - dec)) {
                st->pendRleCount = cplN - (toDecode - dec);
                st->pendRleType  = 3;
                st->pendRleCri   = cri;
                cplN = toDecode - dec;
            }
            for (int j = 0; j < cplN; ++j) {
                WriteCrmPixel(output, dec, bpc, pal, cri);
                ++outPos; ++dec;
            }
        }
    }

    st->pixelsDecoded = outPos;
    st->windowPos = ip;
    st->bitBuf   = buf;
    st->bitCount = bits;
    return CSG_ErrCode::kOk;
}

//=============================================================================
// CAS=2: DEFLATE — self-describing Huffman + MiniLZ77
// On first call: Huffman-decode the entire stream into a temp buffer,
// then switch to MiniLZ77 mode. The temp buffer is freed by the caller.
//=============================================================================
static CSG_ErrCode DecodeDEFLATE(CSGDecoderState* st,
                                  uint8_t* output, int toDecode) {
    // On first call (pixelsDecoded==0), Huffman-decode the stream
    if (st->pixelsDecoded == 0) {
        const uint8_t* input = st->stream;
        size_t inLen = st->streamSize;
        size_t pos = 0;

        // origCount
        if (pos + 4 > inLen) return CSG_ErrCode::kErrHuffHeader;
        uint32_t origCount = static_cast<uint32_t>(input[pos])
            | (static_cast<uint32_t>(input[pos+1]) << 8)
            | (static_cast<uint32_t>(input[pos+2]) << 16)
            | (static_cast<uint32_t>(input[pos+3]) << 24);
        pos += 4;
        if (origCount > kCsgMaxWidth * kCsgMaxHeight) return CSG_ErrCode::kErrBufferShort;

        // maxLen
        if (pos >= inLen) return CSG_ErrCode::kErrHuffHeader;
        int maxLen = input[pos++];
        if (maxLen == 0 || maxLen > 32) return CSG_ErrCode::kErrHuffHeader;

        // count per length
        int cntAt[33] = {};
        int totalSyms = 0;
        for (int l = 1; l <= maxLen; ++l) {
            if (pos + 2 > inLen) return CSG_ErrCode::kErrHuffHeader;
            cntAt[l] = static_cast<int>(input[pos]) | (static_cast<int>(input[pos+1]) << 8);
            pos += 2;
            totalSyms += cntAt[l];
        }
        if (totalSyms > 256) return CSG_ErrCode::kErrHuffHeader;

        // symbol table
        if (pos + totalSyms > inLen) return CSG_ErrCode::kErrHuffHeader;
        uint8_t symTable[256];
        for (int k = 0; k < totalSyms; ++k)
            symTable[k] = input[pos++];

        // bitstream length
        if (pos + 4 > inLen) return CSG_ErrCode::kErrHuffHeader;
        uint32_t bsLen = static_cast<uint32_t>(input[pos])
            | (static_cast<uint32_t>(input[pos+1]) << 8)
            | (static_cast<uint32_t>(input[pos+2]) << 16)
            | (static_cast<uint32_t>(input[pos+3]) << 24);
        pos += 4;
        if (pos + bsLen > inLen) return CSG_ErrCode::kErrHuffHeader;

        GUI_HMEM hRaw = GUI_ALLOC_AllocZero(static_cast<int>(origCount));
        if (hRaw == 0) return CSG_ErrCode::kErrMallocFailed;
        uint8_t* rawBuf = static_cast<uint8_t*>(GUI_ALLOC_h2p(hRaw));
        st->deflateBufHandle = static_cast<size_t>(hRaw);

        // Rebuild canonical decode table
        struct { uint32_t code; int len; uint8_t sym; } table[256];
        int tableSize = 0;
        uint32_t cw = 0;
        int prevL = 0;
        int symIdx = 0;
        for (int l = 1; l <= maxLen; ++l) {
            if (cntAt[l] == 0) continue;
            if (prevL > 0) cw <<= (l - prevL);
            for (int k = 0; k < cntAt[l]; ++k) {
                table[tableSize++] = {cw, l, symTable[symIdx++]};
                ++cw;
            }
            prevL = l;
        }

        // MSB-first decode
        const uint8_t* bs = input + pos;
        int outPos = 0;
        uint32_t accum = 0;
        int accumBits = 0;
        size_t bytePos = 0;

        while (outPos < static_cast<int>(origCount)) {
            while (accumBits <= 24 && bytePos < bsLen) {
                accum = (accum << 8) | bs[bytePos++];
                accumBits += 8;
            }
            if (accumBits == 0) break;
            bool found = false;
            for (int i = 0; i < tableSize; ++i) {
                if (table[i].len > accumBits) continue;
                uint32_t cand = (accum >> (accumBits - table[i].len))
                              & ((1u << table[i].len) - 1);
                if (cand == table[i].code) {
                    rawBuf[outPos++] = table[i].sym;
                    accumBits -= table[i].len;
                    accum &= (1u << accumBits) - 1;
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }

        st->stream     = rawBuf;
        st->windowPos  = 0;
        st->bitBuf     = 0;
        st->bitCount   = 8;
        st->cas        = 3;
        st->windowSize = static_cast<int>(origCount);
    }

    return DecodeMiniLZ77(st, output, toDecode);
}

//=============================================================================
// Public: CsgDecodePixels — CAS dispatch
//=============================================================================
CSG_ErrCode CsgDecodePixels(CSGDecoderState* state,
                             uint8_t* output,
                             int requestedPixels) {
    if (!state || !output || !state->stream)
        return CSG_ErrCode::kErrSrcImgNull;
    if (requestedPixels <= 0)
        return CSG_ErrCode::kOk;

    int totalPixels = state->width * state->height;
    int remaining   = totalPixels - state->pixelsDecoded;
    int toDecode    = Min(requestedPixels, remaining);
    if (toDecode == 0) return CSG_ErrCode::kOk;

    // For CAS=3 (MiniLZ77), use the battle-tested inline path (no helper).
    // For CAS=0/1/2, dispatch to new streaming decoders.
    if (state->cas == 3) {
        // === Inline MiniLZ77 — with pending-match carry-over ===
        const uint8_t* input = state->stream;
        int     ip      = state->windowPos;
        uint8_t ctrl    = static_cast<uint8_t>(state->bitBuf);
        int     tokenIdx = state->bitCount;
        int     outPos  = state->pixelsDecoded;
        uint8_t* win    = state->window;
        int     bpc     = state->bpc;
        const uint8_t* pal = state->palette;
        int     dec     = 0;

        // Resume pending match from previous batch
        if (state->pendMatchLen > 0) {
            int ref = state->pendRef;
            for (int j = 0; j < state->pendMatchLen && dec < toDecode; ++j) {
                uint8_t cri = win[(ref + j) % CSG_DEC_WINDOW_BYTES];
                win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
                if (pal && bpc > 0) {
                    for (int k = 0; k < bpc; ++k)
                        output[dec * bpc + k] = pal[cri * bpc + k];
                } else {
                    output[dec] = cri;
                }
                ++outPos; ++dec;
            }
            // Track remaining pending match
            int consumed = dec;
            state->pendMatchLen -= consumed;
            state->pendRef += consumed;
        }

        while (dec < toDecode) {
            if (tokenIdx >= 8) {
                ctrl     = input[ip++];
                tokenIdx = 0;
            }
            uint8_t cri;
            if (ctrl & (1u << tokenIdx)) {
                int encLen   = input[ip++];
                int encDist  = input[ip++];
                int matchLen = encLen + 3;
                int dist     = encDist + 1;
                if (dist > outPos) return CSG_ErrCode::kErrLz77DistErr;
                int ref = outPos - dist;
                int j = 0;
                for (; j < matchLen && dec < toDecode; ++j) {
                    cri = win[(ref + j) % CSG_DEC_WINDOW_BYTES];
                    win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
                    if (pal && bpc > 0) {
                        for (int k = 0; k < bpc; ++k)
                            output[dec * bpc + k] = pal[cri * bpc + k];
                    } else {
                        output[dec] = cri;
                    }
                    ++outPos; ++dec;
                }
                // Save pending match if truncated by batch limit
                if (j < matchLen) {
                    state->pendMatchLen = matchLen - j;
                    state->pendRef     = ref + j;
                }
            } else {
                cri = input[ip++];
                win[outPos % CSG_DEC_WINDOW_BYTES] = cri;
                if (pal && bpc > 0) {
                    for (int k = 0; k < bpc; ++k)
                        output[dec * bpc + k] = pal[cri * bpc + k];
                } else {
                    output[dec] = cri;
                }
                ++outPos; ++dec;
            }
            ++tokenIdx;
        }

        state->pixelsDecoded = outPos;
        state->bitBuf   = ctrl;
        state->bitCount = tokenIdx;
        state->windowPos = ip;
        state->linePos += dec;
        if (state->linePos >= state->width) {
            state->linePos -= state->width;
            ++state->rowCount;
        }
        return CSG_ErrCode::kOk;
    }

    // Dispatch CAS=0/1/2 to new streaming helpers
    CSG_ErrCode err;
    switch (state->cas) {
        case 0: err = DecodeNone(state, output, toDecode); break;
        case 1: err = DecodeRLE(state, output, toDecode); break;
        case 2: err = DecodeDEFLATE(state, output, toDecode); break;
        default: return CSG_ErrCode::kErrCasInvalid;
    }

    if (err == CSG_ErrCode::kOk) {
        state->linePos += (state->pixelsDecoded - totalPixels + remaining);
        while (state->linePos >= state->width) {
            state->linePos -= state->width;
            ++state->rowCount;
        }
    }
    return err;
}
