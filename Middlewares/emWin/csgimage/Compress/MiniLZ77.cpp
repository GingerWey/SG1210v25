//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : MiniLZ77.cpp
 Version     : V1.51
 By          : Wey. Silver Grid

 Description : MiniLZ77 codec — VP-style 8-token control groups, max match=258,
               max lookback=128.  Circular window indexing for large images.

 Date        : 2026.06.26 (V1.51 — circular window: win[pos % CSG_DEC_WINDOW_BYTES])
              2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#include "Compress/MiniLZ77.h"
#include <cstring>
#include <algorithm>

// ============================================================================
// Constants
// ============================================================================

static constexpr int MIN_MATCH        = 3;
static constexpr int MAX_MATCH        = 258;
static constexpr int TOKENS_PER_GROUP = 8;

// ============================================================================
// Construction & Configuration
// ============================================================================

MiniLZ77Codec::MiniLZ77Codec() : lenBits_(0) {}

void MiniLZ77Codec::UpdateCfg() { cfg_ = kLz77CalTable[cal_]; }

const CSG_LZ77_Cfg& MiniLZ77Codec::GetCfg() const { return cfg_; }
int MiniLZ77Codec::LenBits() const { return lenBits_; }

// ============================================================================
// Compress — standalone MiniLZ77 (CAS=3)
// Full linear scan with 256-byte window (covers 5+ rows for 48×48 images)
// ============================================================================

// Find best match at position pos. Returns {len, dist}.
static std::pair<int,int> findMatch(const uint8_t* p, int pos, int n, int window) {
    int wStart = std::max(0, pos - window);
    int bestLen = 0, bestDist = 0;
    for (int j = wStart; j < pos; ++j) {
        int ml = 0;
        int maxCheck = std::min(MAX_MATCH, n - pos);
        while (ml < maxCheck && p[pos + ml] == p[j + ml]) ++ml;
        if (ml > bestLen || (ml == bestLen && (pos - j) < bestDist)) {
            bestLen = ml; bestDist = pos - j;
        }
    }
    return {bestLen, bestDist};
}

std::vector<uint8_t> MiniLZ77Codec::Compress(
    const uint8_t* pixels, int width, int height, int crn, ColorMode cm) {

    int totalPixels = width * height;
    int bpc = BytesPerColor(cm);
    int pixelStride = (crn > 0) ? 1 : bpc;
    int n = totalPixels * pixelStride;
    if (n == 0) return {};

    int window = (n < 256) ? n : 256;
    std::vector<uint8_t> out;
    int i = 0;

    while (i < n) {
        uint8_t ctrl = 0;
        std::vector<uint8_t> tokens;
        int tcnt = 0;

        while (tcnt < TOKENS_PER_GROUP && i < n) {
            auto [bestLen, bestDist] = findMatch(pixels, i, n, window);

            // Lazy matching: if next position has a much better match,
            // emit literal now and take the better match next iteration.
            if (bestLen >= MIN_MATCH && bestLen < 32 && i + 1 < n) {
                auto [nextLen, nextDist] = findMatch(pixels, i + 1, n, window);
                if (nextLen > bestLen + 1) {
                    bestLen = 0;
                }
            }

            if (bestLen >= MIN_MATCH) {
                tokens.push_back(static_cast<uint8_t>(bestLen - MIN_MATCH));
                tokens.push_back(static_cast<uint8_t>(bestDist - 1));
                ctrl |= (1u << tcnt);
                i += bestLen;
            } else {
                tokens.push_back(pixels[i]);
                ++i;
            }
            ++tcnt;
        }

        out.push_back(ctrl);
        out.insert(out.end(), tokens.begin(), tokens.end());
    }
    return out;
}

// ============================================================================
// CompressToSymbols — for DEFLATE pipeline (CAS=2)
// ============================================================================

std::vector<MiniLZ77Codec::Lz77Symbol> MiniLZ77Codec::CompressToSymbols(
    const uint8_t* pixels, int width, int height) {

    int n = width * height;
    int window = n;
    std::vector<Lz77Symbol> symbols;
    int i = 0;

    while (i < n) {
        int wStart = std::max(0, i - window);
        int bestLen = 0, bestDist = 0;

        for (int j = wStart; j < i; ++j) {
            int ml = 0;
            int maxCheck = std::min(MAX_MATCH, n - i);
            while (ml < maxCheck
                   && pixels[i + ml] == pixels[j + ml]) ++ml;
            if (ml > bestLen
                || (ml == bestLen && (i - j) < bestDist)) {
                bestLen = ml; bestDist = i - j;
            }
        }

        Lz77Symbol sym;
        if (bestLen >= MIN_MATCH) {
            sym.isLiteral = false;
            sym.distance  = static_cast<uint16_t>(bestDist);
            sym.length    = static_cast<uint16_t>(bestLen);
        } else {
            sym.isLiteral = true;
            sym.literal   = pixels[i];
        }
        symbols.push_back(sym);
        i += (bestLen >= MIN_MATCH) ? bestLen : 1;
    }
    return symbols;
}

// ============================================================================
// Decompress — standalone MiniLZ77 (CAS=3)
// ============================================================================

CSG_ErrCode MiniLZ77Codec::Decompress(
    const uint8_t* input, size_t inputLen,
    uint8_t* output, int width, int height, int crn, ColorMode cm) {

    (void)crn; (void)cm;

    int totalBytes = width * height * ((crn > 0) ? 1 : BytesPerColor((ColorMode)cm));
    int outPos = 0;
    size_t ip = 0;

    while (outPos < totalBytes && ip < inputLen) {
        if (ip >= inputLen) break;
        uint8_t ctrl = input[ip++];

        for (int t = 0; t < TOKENS_PER_GROUP && outPos < totalBytes; ++t) {
            if (ip >= inputLen) break;

            if (ctrl & (1u << t)) {
                if (ip + 1 >= inputLen)
                    return CSG_ErrCode::kErrLz77GroupErr;
                int encLen  = input[ip++];
                int encDist = input[ip++];
                int matchLen = encLen + MIN_MATCH;
                int dist     = encDist + 1;

                if (dist < 1 || dist > outPos)
                    return CSG_ErrCode::kErrLz77DistErr;
                if (outPos + matchLen > totalBytes)
                    return CSG_ErrCode::kErrLz77GroupErr;

                int ref = outPos - dist;
                for (int j = 0; j < matchLen; ++j)
                    output[outPos++] = output[ref + j];
            } else {
                output[outPos++] = input[ip++];
            }
        }
    }
    return (outPos == totalBytes) ? CSG_ErrCode::kOk : CSG_ErrCode::kErrLz77GroupErr;
}
