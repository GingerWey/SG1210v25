//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : Huffman.cpp
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : Huffman codec — self-describing header, MSB-first bitstream
               encode/decode, canonical codeword assignment.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#include "Compress/Huffman.h"
#include <cstring>
#include <algorithm>
#include <queue>
#include <functional>

// ============================================================================
// Construction
// ============================================================================

HuffmanCodec::HuffmanCodec()
    : pureMaxBits_(0), litLenMaxBits_(0), distMaxBits_(0),
      tablesBuilt_(false) {
    std::memset(pureLengths_, 0, sizeof(pureLengths_));
    std::memset(litLenLengths_, 0, sizeof(litLenLengths_));
    std::memset(distLengths_, 0, sizeof(distLengths_));
    std::memset(pureCodewords_, 0, sizeof(pureCodewords_));
    std::memset(litLenCodewords_, 0, sizeof(litLenCodewords_));
    std::memset(distCodewords_, 0, sizeof(distCodewords_));
}

// ============================================================================
// Build code tables from CAL
// ============================================================================

void HuffmanCodec::BuildCodeTables() {
    int C = kHuffCalTable[cal_];
    pureMaxBits_ = C;
    litLenMaxBits_ = C;
    distMaxBits_ = kHuffDistCodeLen;

    // Pure Huffman (256 symbols)
    if (!HuffGenLengthsPure(cal_, pureLengths_)) {
        for (int i = 0; i < kHuffPureSymbols; ++i)
            pureLengths_[i] = kHuffPureBaseCodeLen;
    }
    HuffAssignCodewords(pureLengths_, kHuffPureSymbols, pureCodewords_, C);
    HuffBuildDecodeTable(pureCodewords_, pureLengths_, kHuffPureSymbols,
                         pureDecodeTable_, pureMaxBits_);

    // DEFLATE lit/len (286 symbols, CAL ≥ 3)
    if (cal_ >= 3) {
        HuffGenLengthsDeflateLitLen(cal_, litLenLengths_);
        HuffAssignCodewords(litLenLengths_, kHuffLitLenSymbols,
                            litLenCodewords_, C);
        HuffBuildDecodeTable(litLenCodewords_, litLenLengths_,
                             kHuffLitLenSymbols,
                             litLenDecodeTable_, litLenMaxBits_);
    } else {
        for (int i = 0; i < kHuffLitLenSymbols; ++i)
            litLenLengths_[i] = kHuffPureBaseCodeLen;
        HuffAssignCodewords(litLenLengths_, kHuffLitLenSymbols,
                            litLenCodewords_, kHuffPureBaseCodeLen);
        HuffBuildDecodeTable(litLenCodewords_, litLenLengths_,
                             kHuffLitLenSymbols,
                             litLenDecodeTable_, kHuffPureBaseCodeLen);
        litLenMaxBits_ = kHuffPureBaseCodeLen;
    }

    // Distance table (32 symbols, always length 5)
    HuffGenLengthsDeflateDist(distLengths_);
    HuffAssignCodewords(distLengths_, kHuffDistSymbols, distCodewords_,
                        kHuffDistCodeLen);
    HuffBuildDecodeTable(distCodewords_, distLengths_, kHuffDistSymbols,
                         distDecodeTable_, distMaxBits_);

    tablesBuilt_ = true;
}

// ============================================================================
// Encode symbols into MSB-first bitstream (template for uint8_t / uint16_t)
// ============================================================================

template <typename TSym>
static std::vector<uint8_t> EncodeSymbolsImpl(
    const TSym* symbols, int count,
    const uint16_t* codewords, const uint8_t* lengths) {

    std::vector<uint8_t> output;
    uint32_t bitBuf = 0;
    int bitCount = 0;

    for (int i = 0; i < count; ++i) {
        unsigned sym = static_cast<unsigned>(symbols[i]);
        uint8_t len = lengths[sym];
        uint16_t cw  = codewords[sym];
        if (len == 0) continue;

        for (int b = len - 1; b >= 0; --b) {
            if (cw & (1u << b))
                bitBuf |= (1u << bitCount);
            ++bitCount;
            if (bitCount == 8) {
                output.push_back(static_cast<uint8_t>(bitBuf & 0xFF));
                bitBuf = 0;
                bitCount = 0;
            }
        }
    }
    if (bitCount > 0)
        output.push_back(static_cast<uint8_t>(bitBuf & 0xFF));
    return output;
}

// ============================================================================
// Decode symbols from MSB-first bitstream (uint8_t or uint16_t output)
// ============================================================================

template <typename TSym>
static CSG_ErrCode DecodeSymbolsImpl(
    const uint8_t* input, size_t inputLen,
    TSym* symbols, int maxSymbols, int* outCount,
    const uint32_t* decodeTable, int maxBits) {

    *outCount = 0;
    size_t bytePos = 0;
    int bitPos = 0;

    while (*outCount < maxSymbols && bytePos < inputLen) {
        uint32_t peek = 0;
        int bitsCollected = 0;
        size_t tmpByte = bytePos;
        int tmpBit = bitPos;

        while (bitsCollected < maxBits && tmpByte < inputLen) {
            peek = (peek << 1) | ((input[tmpByte] >> tmpBit) & 1);
            ++bitsCollected; ++tmpBit;
            if (tmpBit == 8) { tmpBit = 0; ++tmpByte; }
        }

        if (bitsCollected < maxBits && bitsCollected == 0)
            break;

        uint16_t idx = static_cast<uint16_t>(peek & ((1u << maxBits) - 1));
        uint32_t entry = decodeTable[idx];

        if (entry == kHuffTableSentinel)
            return CSG_ErrCode::kErrHuffNoMatch;

        uint8_t  symLen = entry & 0xFF;
        uint16_t sym    = static_cast<uint16_t>((entry >> 8) & 0xFFFF);

        if (symLen == 0 || symLen > static_cast<uint8_t>(bitsCollected))
            return CSG_ErrCode::kErrHuffNoMatch;

        symbols[*outCount] = static_cast<TSym>(sym);
        (*outCount)++;

        bitPos += symLen;
        while (bitPos >= 8) { bitPos -= 8; ++bytePos; }
    }
    return CSG_ErrCode::kOk;
}

// ============================================================================
// Pure Huffman — frequency-based tree with self-describing output (VP format)
// ============================================================================

std::vector<uint8_t> HuffmanCodec::Compress(
    const uint8_t* pixels, int width, int height, int crn, ColorMode cm) {

    int totalPixels = width * height;
    int bpc = BytesPerColor(cm);
    int pixelStride = (crn > 0) ? 1 : bpc;

    // Flatten pixel data to byte sequence
    std::vector<uint8_t> data;
    data.reserve(totalPixels * ((crn > 0) ? 1 : bpc));
    for (int i = 0; i < totalPixels; ++i) {
        const uint8_t* p = pixels + i * pixelStride;
        if (crn > 0) data.push_back(*p);
        else for (int j = 0; j < bpc; ++j) data.push_back(p[j]);
    }
    if (data.empty()) return {};

    // Frequency counts
    std::vector<int> freq(256, 0);
    for (uint8_t b : data) freq[b]++;

    // Build Huffman tree
    struct HTNode { int f, left, right, sym; };
    std::vector<HTNode> pool;
    using PQItem = std::pair<int,int>;
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;

    for (int s = 0; s < 256; ++s) {
        if (freq[s] > 0) {
            int id = static_cast<int>(pool.size());
            pool.push_back({freq[s], -1, -1, s});
            pq.push({freq[s], id});
        }
    }
    if (pq.empty()) return {};

    // Single-symbol edge case
    if (pq.size() == 1) {
        int s = pool[pq.top().second].sym;
        std::vector<uint8_t> out;
        uint32_t n = static_cast<uint32_t>(data.size());
        out.push_back(static_cast<uint8_t>(n));
        out.push_back(static_cast<uint8_t>(n >> 8));
        out.push_back(static_cast<uint8_t>(n >> 16));
        out.push_back(static_cast<uint8_t>(n >> 24));
        out.push_back(1);          // maxLen = 1
        out.push_back(1);          // countAtLen[1] = 1 (low)
        out.push_back(0);          // countAtLen[1] = 1 (high)
        out.push_back(static_cast<uint8_t>(s));
        out.push_back(0); out.push_back(0); out.push_back(0); out.push_back(0); // bsLen=0
        return out;
    }

    // Merge nodes
    while (pq.size() > 1) {
        int f1 = pq.top().first, i1 = pq.top().second; pq.pop();
        int f2 = pq.top().first, i2 = pq.top().second; pq.pop();
        int p = static_cast<int>(pool.size());
        pool.push_back({f1 + f2, i1, i2, -1});
        pq.push({f1 + f2, p});
    }

    // Assign code lengths via DFS
    std::vector<int> lengths(256, 0);
    std::function<void(int,int)> dfs = [&](int node, int depth) {
        if (pool[node].sym >= 0) { lengths[pool[node].sym] = depth; return; }
        if (pool[node].left >= 0) dfs(pool[node].left, depth + 1);
        if (pool[node].right >= 0) dfs(pool[node].right, depth + 1);
    };
    dfs(pq.top().second, 0);

    // Collect used symbols, sort canonical
    struct SL { int sym, len; };
    std::vector<SL> sl;
    for (int s = 0; s < 256; ++s)
        if (lengths[s] > 0) sl.push_back({s, lengths[s]});
    std::sort(sl.begin(), sl.end(),
              [](const SL& a, const SL& b) {
                  return a.len != b.len ? a.len < b.len : a.sym < b.sym;
              });

    // Canonical codeword assignment
    std::vector<uint32_t> code(256, 0);
    int maxLen = 0;
    for (auto& x : sl) maxLen = std::max(maxLen, x.len);
    std::vector<int> cntAt(maxLen + 1, 0);
    for (auto& x : sl) cntAt[x.len]++;

    if (!sl.empty()) {
        uint32_t cw = 0;
        int prevLen = sl[0].len;
        for (auto& x : sl) {
            if (x.len > prevLen) { cw <<= (x.len - prevLen); prevLen = x.len; }
            code[x.sym] = cw++;
        }
    }

    // MSB-first bitstream
    std::vector<uint8_t> bs;
    uint8_t cur = 0;
    int bitsLeft = 8;
    for (uint8_t b : data) {
        int L = lengths[b];
        uint32_t C = code[b];
        for (int k = L - 1; k >= 0; --k) {
            cur |= static_cast<uint8_t>(((C >> k) & 1) << (bitsLeft - 1));
            if (--bitsLeft == 0) { bs.push_back(cur); cur = 0; bitsLeft = 8; }
        }
    }
    if (bitsLeft < 8) bs.push_back(cur);

    // Assemble output
    std::vector<uint8_t> out;
    uint32_t n = static_cast<uint32_t>(data.size());
    out.push_back(static_cast<uint8_t>(n));
    out.push_back(static_cast<uint8_t>(n >> 8));
    out.push_back(static_cast<uint8_t>(n >> 16));
    out.push_back(static_cast<uint8_t>(n >> 24));
    out.push_back(static_cast<uint8_t>(maxLen));
    for (int l = 1; l <= maxLen; ++l) {
        uint16_t c = static_cast<uint16_t>(cntAt[l]);
        out.push_back(static_cast<uint8_t>(c));
        out.push_back(static_cast<uint8_t>(c >> 8));
    }
    for (auto& x : sl) out.push_back(static_cast<uint8_t>(x.sym));
    uint32_t bz = static_cast<uint32_t>(bs.size());
    out.push_back(static_cast<uint8_t>(bz));
    out.push_back(static_cast<uint8_t>(bz >> 8));
    out.push_back(static_cast<uint8_t>(bz >> 16));
    out.push_back(static_cast<uint8_t>(bz >> 24));
    out.insert(out.end(), bs.begin(), bs.end());
    return out;
}

CSG_ErrCode HuffmanCodec::Decompress(
    const uint8_t* input, size_t inputLen,
    uint8_t* output, int width, int height, int crn, ColorMode cm) {

    (void)crn; (void)cm; (void)width; (void)height;
    if (inputLen < 5) return CSG_ErrCode::kErrHuffHeader;
    size_t pos = 0;

    // origCount
    uint32_t origCount = static_cast<uint32_t>(input[pos])
        | (static_cast<uint32_t>(input[pos + 1]) << 8)
        | (static_cast<uint32_t>(input[pos + 2]) << 16)
        | (static_cast<uint32_t>(input[pos + 3]) << 24);
    pos += 4;
    if (pos >= inputLen) return CSG_ErrCode::kErrHuffHeader;

    // maxLen
    int maxLen = input[pos++];
    if (maxLen == 0 || maxLen > 32) return CSG_ErrCode::kErrHuffHeader;

    // counts per length
    std::vector<int> cntAt(maxLen + 1, 0);
    for (int l = 1; l <= maxLen; ++l) {
        if (pos + 2 > inputLen) return CSG_ErrCode::kErrHuffHeader;
        cntAt[l] = static_cast<int>(input[pos])
                 | (static_cast<int>(input[pos + 1]) << 8);
        pos += 2;
    }
    int totalSyms = 0;
    for (int l = 1; l <= maxLen; ++l) totalSyms += cntAt[l];
    if (pos + static_cast<size_t>(totalSyms) > inputLen)
        return CSG_ErrCode::kErrHuffHeader;

    // symbol table
    std::vector<uint8_t> symTable(totalSyms);
    for (int k = 0; k < totalSyms; ++k)
        symTable[k] = input[pos++];

    // bitstream length
    if (pos + 4 > inputLen) return CSG_ErrCode::kErrHuffHeader;
    uint32_t bsLen = static_cast<uint32_t>(input[pos])
        | (static_cast<uint32_t>(input[pos + 1]) << 8)
        | (static_cast<uint32_t>(input[pos + 2]) << 16)
        | (static_cast<uint32_t>(input[pos + 3]) << 24);
    pos += 4;
    if (pos + bsLen > inputLen) return CSG_ErrCode::kErrHuffHeader;
    const uint8_t* bs = input + pos;

    // Rebuild decode table
    struct Entry { uint32_t code; int len; uint8_t sym; };
    std::vector<Entry> table;
    {
        uint32_t cw = 0;
        int prevL = 0;
        int symIdx = 0;
        for (int l = 1; l <= maxLen; ++l) {
            if (cntAt[l] == 0) continue;
            if (prevL > 0) cw <<= (l - prevL);
            for (int k = 0; k < cntAt[l]; ++k) {
                table.push_back({cw, l, symTable[symIdx++]});
                ++cw;
            }
            prevL = l;
        }
    }

    // MSB-first decode
    int outPos = 0;
    uint32_t accum = 0;
    int accumBits = 0;
    size_t bytePos = 0;

    auto refill = [&]() {
        while (accumBits <= 24 && bytePos < bsLen) {
            accum = (accum << 8) | bs[bytePos++];
            accumBits += 8;
        }
    };
    refill();

    while (outPos < static_cast<int>(origCount) && accumBits > 0) {
        bool found = false;
        for (const auto& e : table) {
            if (e.len > accumBits) continue;
            uint32_t cand = (accum >> (accumBits - e.len))
                          & ((1u << e.len) - 1);
            if (cand == e.code) {
                output[outPos++] = e.sym;
                accumBits -= e.len;
                accum &= (accumBits < 32) ? ((1u << accumBits) - 1) : ~0u;
                refill();
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    return (outPos >= static_cast<int>(origCount))
        ? CSG_ErrCode::kOk : CSG_ErrCode::kErrBitpackTrunc;
}

// Raw byte Huffman — no pixel layout, just raw data (for DEFLATE pipeline)
// Pass crn=2 so pixelStride=1 (each byte treated as one symbol)
std::vector<uint8_t> HuffmanCodec::CompressRaw(const uint8_t* data, int count) {
    HuffmanCodec huff;
    return huff.Compress(data, 1, count, 2, ColorMode::kRGB565);
}

std::vector<uint8_t> HuffmanCodec::DecompressRaw(const uint8_t* data, size_t len) {
    HuffmanCodec huff;
    return huff.DecompressVec(data, len);
}

// Convenience: decode into vector (sized from self-describing header)
std::vector<uint8_t> HuffmanCodec::DecompressVec(const uint8_t* input, size_t inputLen) {
    if (inputLen < 5) return {};
    uint32_t origCount = (uint32_t)input[0]|((uint32_t)input[1]<<8)
                       |((uint32_t)input[2]<<16)|((uint32_t)input[3]<<24);
    std::vector<uint8_t> out(origCount);
    CSG_ErrCode err = Decompress(input, inputLen, out.data(), 1, (int)origCount, 0, ColorMode::kRGB565);
    if (err != CSG_ErrCode::kOk) out.clear();
    return out;
}

// ============================================================================
// DEFLATE lit/len (uint16_t symbols 0..285)
// ============================================================================

std::vector<uint8_t> HuffmanCodec::CompressLitLen(const uint16_t* symbols, int count) {
    if (!tablesBuilt_) BuildCodeTables();
    return EncodeSymbolsImpl(symbols, count, litLenCodewords_, litLenLengths_);
}

CSG_ErrCode HuffmanCodec::DecompressLitLen(const uint8_t* input, size_t inputLen,
                                            uint16_t* symbols, int maxSymbols,
                                            int* outCount) {
    if (!tablesBuilt_) BuildCodeTables();
    return DecodeSymbolsImpl(input, inputLen, symbols, maxSymbols, outCount,
                              litLenDecodeTable_, litLenMaxBits_);
}

// ============================================================================
// DEFLATE distance (uint8_t symbols 0..31)
// ============================================================================

std::vector<uint8_t> HuffmanCodec::CompressDist(const uint8_t* symbols, int count) {
    if (!tablesBuilt_) BuildCodeTables();
    return EncodeSymbolsImpl(symbols, count, distCodewords_, distLengths_);
}

CSG_ErrCode HuffmanCodec::DecompressDist(const uint8_t* input, size_t inputLen,
                                          uint8_t* symbols, int maxSymbols,
                                          int* outCount) {
    if (!tablesBuilt_) BuildCodeTables();
    return DecodeSymbolsImpl(input, inputLen, symbols, maxSymbols, outCount,
                              distDecodeTable_, distMaxBits_);
}
