//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CSGCommon.cpp
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : CRC16, string helpers, file I/O, Huffman code-length generation,
               code assignment, and decode-table builder implementations.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#include "CSGCommon.h"

// ============================================================================
// CRC16 — nibble-table driven (precomputed for poly 0x8005)
// ============================================================================

static const uint16_t kCrc16NibbleTable[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

uint16_t Crc16(const void* data, uint32_t numBytes) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    uint16_t result = kCrc16Init;

    for (uint32_t i = 0; i < numBytes; ++i) {
        uint8_t c = *p++;
        result = (result >> 4) ^
                 kCrc16NibbleTable[(c ^ result) & 0x0F];
        result = (result >> 4) ^
                 kCrc16NibbleTable[((c >> 4) ^ result) & 0x0F];
    }

    return result ^ kCrc16FinalXor;  // Ones' complement
}

// ============================================================================
// Color mode name lookup
// ============================================================================

const std::string& ColorModeName(ColorMode cm) {
    static const std::string kRgb565  = "RGB565";
    static const std::string kBgr565  = "BGR565";
    static const std::string kRgb666  = "RGB666";
    static const std::string kRgb888  = "RGB888";
    static const std::string kArgb8888 = "ARGB8888";
    static const std::string kUnknown  = "Unknown";

    switch (cm) {
        case ColorMode::kRGB565:  return kRgb565;
        case ColorMode::kBGR565:  return kBgr565;
        case ColorMode::kRGB666:  return kRgb666;
        case ColorMode::kRGB888:  return kRgb888;
        case ColorMode::kRGB8888: return kArgb8888;
        default:                  return kUnknown;
    }
}

// ============================================================================
// Compression algorithm name lookup
// ============================================================================

const std::string& CompressModeName(CompressAlgorithm algo) {
    static const std::string kNone     = "None";
    static const std::string kRle      = "RLE";
    static const std::string kDeflate  = "DEFLATE";
    static const std::string kMiniLz77 = "MiniLZ77";
    static const std::string kHuffman  = "Huffman";
    static const std::string kUnknown  = "Unknown";

    switch (algo) {
        case CompressAlgorithm::kNone:     return kNone;
        case CompressAlgorithm::kRLE:      return kRle;
        case CompressAlgorithm::kDEFLATE:  return kDeflate;
        case CompressAlgorithm::kMiniLZ77: return kMiniLz77;
        case CompressAlgorithm::kHuffman:  return kHuffman;
        default:                           return kUnknown;
    }
}

// ============================================================================
// Error code to string
// ============================================================================

const char* CsgGetErrStr(CSG_ErrCode code) noexcept {
    switch (code) {
        case CSG_ErrCode::kOk:
            return "CSG_OK: Operation success";

        case CSG_ErrCode::kErrFileOpen:
            return "CSG_ERR_FILE_OPEN: Failed to open file";
        case CSG_ErrCode::kErrFileRead:
            return "CSG_ERR_FILE_READ: File read error";
        case CSG_ErrCode::kErrFileWrite:
            return "CSG_ERR_FILE_WRITE: File write error";
        case CSG_ErrCode::kErrFileEof:
            return "CSG_ERR_FILE_EOF: Unexpected end of file";
        case CSG_ErrCode::kErrGlobalMagic:
            return "CSG_ERR_GLOBAL_MAGIC: Global magic 'CG' mismatch";
        case CSG_ErrCode::kErrGlobalCrc:
            return "CSG_ERR_GLOBAL_CRC: Global CRC16 verify failed";
        case CSG_ErrCode::kErrVersionInvalid:
            return "CSG_ERR_VERSION_INVALID: Unsupported CSG version";
        case CSG_ErrCode::kErrPicCountZero:
            return "CSG_ERR_PICCOUNT_ZERO: Zero pictures in atlas";
        case CSG_ErrCode::kErrOffsetTableBroken:
            return "CSG_ERR_OFFSET_TABLE_BROKEN: Offset table corrupted";

        case CSG_ErrCode::kErrPicMagic:
            return "CSG_ERR_PIC_MAGIC: Sub-picture magic 'Vx' mismatch";
        case CSG_ErrCode::kErrPicCrc:
            return "CSG_ERR_PIC_CRC: Picture CRC16 verify failed";
        case CSG_ErrCode::kErrPicSizeOverflow:
            return "CSG_ERR_PIC_SIZE_OVERFLOW: Picture size out of bounds";
        case CSG_ErrCode::kErrWidthInvalid:
            return "CSG_ERR_WIDTH_INVALID: Invalid width (10..256)";
        case CSG_ErrCode::kErrHeightInvalid:
            return "CSG_ERR_HEIGHT_INVALID: Invalid height (10..256)";
        case CSG_ErrCode::kErrCrmInvalid:
            return "CSG_ERR_CRM_INVALID: Unknown color mode CRM";
        case CSG_ErrCode::kErrCrnInvalid:
            return "CSG_ERR_CRN_INVALID: Illegal CRN value";
        case CSG_ErrCode::kErrCasInvalid:
            return "CSG_ERR_CAS_INVALID: Unknown compression algorithm";
        case CSG_ErrCode::kErrPposAlign:
            return "CSG_ERR_PPOS_ALIGN: Palette offset PPOS unaligned";
        case CSG_ErrCode::kErrDposAlign:
            return "CSG_ERR_DPOS_ALIGN: Data offset DPOS unaligned";
        case CSG_ErrCode::kErrDposOverflow:
            return "CSG_ERR_DPOS_OVERFLOW: DPOS exceeds picture size";
        case CSG_ErrCode::kErrSinglePicTooLarge:
            return "CSG_ERR_SINGLE_PIC_TOO_LARGE: Picture > 65500 bytes";
        case CSG_ErrCode::kErrPicCountLimit:
            return "CSG_ERR_PIC_COUNT_LIMIT: Exceeded max 100 pictures";

        case CSG_ErrCode::kErrPaletteMiss:
            return "CSG_ERR_PALETTE_MISS: Palette data missing";
        case CSG_ErrCode::kErrPaletteLenErr:
            return "CSG_ERR_PALETTE_LEN_ERR: Palette length mismatch";
        case CSG_ErrCode::kErrPaletteRead:
            return "CSG_ERR_PALETTE_READ: Palette data truncated";
        case CSG_ErrCode::kErrTransparentColorInvalid:
            return "CSG_ERR_TRANSPARENT_COLOR_INVALID: Invalid transparent color";

        case CSG_ErrCode::kErrCompStreamEmpty:
            return "CSG_ERR_COMP_STREAM_EMPTY: Empty compressed stream";
        case CSG_ErrCode::kErrRleFrameErr:
            return "CSG_ERR_RLE_FRAME_ERR: Invalid RLE frame";
        case CSG_ErrCode::kErrRleCriMiss:
            return "CSG_ERR_RLE_CRI_MISS: Missing CRI in RLE frame";
        case CSG_ErrCode::kErrHuffHeader:
            return "CSG_ERR_HUFF_HEADER: Damaged Huffman header";
        case CSG_ErrCode::kErrHuffNoMatch:
            return "CSG_ERR_HUFF_NO_MATCH: No matching Huffman codeword";
        case CSG_ErrCode::kErrLz77GroupErr:
            return "CSG_ERR_LZ77_GROUP_ERR: MiniLZ77 control group invalid";
        case CSG_ErrCode::kErrLz77DistErr:
            return "CSG_ERR_LZ77_DIST_ERR: LZ77 distance out of window";
        case CSG_ErrCode::kErrBitpackTrunc:
            return "CSG_ERR_BITPACK_TRUNC: Bitstream truncated";
        case CSG_ErrCode::kErrLz77CalMiss:
            return "CSG_ERR_LZ77_CAL_MISS: Missing CAL for LZ77 config";
        case CSG_ErrCode::kErrRleCplTrunc:
            return "CSG_ERR_RLE_CPL_TRUNC: Truncated CPL long frame";

        case CSG_ErrCode::kErrMallocFailed:
            return "CSG_ERR_MALLOC_FAILED: Memory allocation failed";
        case CSG_ErrCode::kErrBufferShort:
            return "CSG_ERR_BUFFER_SHORT: Output buffer too small";

        case CSG_ErrCode::kErrSrcImgNull:
            return "CSG_ERR_SRC_IMG_NULL: Source image is null";
        case CSG_ErrCode::kErrQuantFail:
            return "CSG_ERR_QUANT_FAIL: Color quantization failed";
        case CSG_ErrCode::kErrCalUndefined:
            return "CSG_ERR_CAL_UNDEFINED: Undefined CAL level (0..7)";
        case CSG_ErrCode::kErrExportCppFail:
            return "CSG_ERR_EXPORT_CPP_FAIL: Export .h/.cpp failed";
        case CSG_ErrCode::kErrImgDamaged:
            return "CSG_ERR_IMG_DAMAGED: Source image corrupted";
        case CSG_ErrCode::kErrProjectXmlBroken:
            return "CSG_ERR_PROJECT_XML_BROKEN: .csgprj XML parse failed";

        case CSG_ErrCode::kErrStreamLimit:
            return "CSG_ERR_STREAM_LIMIT: Stream decode limit exceeded";
        case CSG_ErrCode::kErrOutOfRange:
            return "CSG_ERR_OUT_OF_RANGE: Picture index out of range";
        case CSG_ErrCode::kErrExtensionBitUnknown:
            return "CSG_ERR_EXTENSION_BIT_UNKNOWN: Unknown extension flag";

        case CSG_ErrCode::kErrReservedStart:
        default:
            return "CSG_UNKNOWN_ERR: Undefined error code";
    }
}

// ============================================================================
// Current time string (Windows) / Platform stub
// ============================================================================

std::string CurrentTimeString() {
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << st.wYear << "-"
        << std::setw(2) << std::setfill('0') << st.wMonth << "-"
        << std::setw(2) << std::setfill('0') << st.wDay << " "
        << std::setw(2) << std::setfill('0') << st.wHour << ":"
        << std::setw(2) << std::setfill('0') << st.wMinute << ":"
        << std::setw(2) << std::setfill('0') << st.wSecond;
    return oss.str();
#else
    return "1970-01-01 00:00:00";  // Platform stub — encoder only used on Windows
#endif
}

// ============================================================================
// Wide-string to UTF-8
// ============================================================================

std::string WideToUtf8(const std::wstring& ws) {
    if (ws.empty()) return {};
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                                  static_cast<int>(ws.size()),
                                  nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                        static_cast<int>(ws.size()),
                        &result[0], len, nullptr, nullptr);
    return result;
#else
    // Fallback for non-Windows: assume ASCII-compatible narrow encoding
    return std::string(ws.begin(), ws.end());
#endif
}

// ============================================================================
// File I/O
// ============================================================================

// Encoder-only: file I/O uses std::wstring paths (Windows convention).
// These functions throw on failure; callers catch the exceptions.
// On non-Windows platforms these are no-ops — CSG files are built in memory.

void WriteBinaryFile(const std::wstring& path,
                     const std::vector<uint8_t>& bytes) {
#ifdef _WIN32
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        throw std::runtime_error("Cannot write file: " + WideToUtf8(path));
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
#else
    (void)path; (void)bytes;  // No-op stub for non-Windows
#endif
}

void WriteTextFile(const std::wstring& path, const std::string& text) {
#ifdef _WIN32
    std::ofstream f(path, std::ios::trunc);
    if (!f)
        throw std::runtime_error("Cannot write file: " + WideToUtf8(path));
    f << text;
#else
    (void)path; (void)text;   // No-op stub for non-Windows
#endif
}

// ============================================================================
// Caption generation for .cpp output
// ============================================================================

std::string GenerateCppCaption(const std::string& symbolName) {
    int major = (kEncoderVersion >> 24) & 0xFF;
    int minor = (kEncoderVersion >> 16) & 0xFF;
    int build = kEncoderVersion & 0xFFFF;

    std::ostringstream ss;
    ss << "// ------------------------------------------------------------\n"
       << "/*\n"
       << "   " << symbolName << ".cpp\n"
       << "   CSG image data\n\n"
       << "   Encoder    name: " << kEncoderName << "\n"
       << "   Encoder version: " << major << "."
       << minor << "." << std::hex << std::setw(4)
       << std::setfill('0') << build << std::dec << "\n\n"
       << "   By Wey.   Silver Grid 2026\n"
       << "   Time: " << CurrentTimeString() << "\n"
       << "*/\n"
       << "// ------------------------------------------------------------\n"
       << "#include \"" << symbolName << ".h\"\n\n";
    return ss.str();
}

// ============================================================================
// Generate .h file with header guard
// ============================================================================

std::string GenerateHeaderGuard(const std::string& fileName,
                                const std::string& symbolName,
                                const std::vector<std::string>& picNames) {
    std::string guard = symbolName + "_H";
    int major = (kEncoderVersion >> 24) & 0xFF;
    int minor = (kEncoderVersion >> 16) & 0xFF;
    int build = kEncoderVersion & 0xFFFF;

    std::ostringstream ss;
    ss << "// ------------------------------------------------------------\n"
       << "/*\n"
       << "   " << symbolName << ".h\n"
       << "   CSG image data\n\n"
       << "   Encoder    name: " << kEncoderName << "\n"
       << "   Encoder version: " << major << "."
       << minor << "." << std::hex << std::setw(4)
       << std::setfill('0') << build << std::dec << "\n\n"
       << "   By Wey.   Silver Grid 2026\n"
       << "   Time: " << CurrentTimeString() << "\n"
       << "*/\n"
       << "// ------------------------------------------------------------\n"
       << "#ifndef " << guard << "\n"
       << "#define " << guard << "\n\n"
       << "#include \"GUIPicture.h\"\n\n"
       << "// ============================================================\n"
       << "#ifdef __cplusplus\n"
       << "extern \"C\" {\n"
       << "#endif\n\n";

    for (const auto& name : picNames) {
        ss << "// ------------------------------------------------------------\n"
           << "// CSG Image: " << name << "\n"
           << "extern const TGUIPicture " << name << ";\n\n";
    }

    ss << "// ============================================================\n"
       << "#ifdef __cplusplus\n"
       << "}\n"
       << "#endif\n"
       << "// ============================================================\n"
       << "#endif  // " << guard << "\n";

    return ss.str();
}

// ============================================================================
// Huffman code-length generation
// ============================================================================

bool HuffGenLengthsPure(int cal, uint8_t lengths[kHuffPureSymbols]) {
    if (cal < 2 || cal > kCalMax) return false;
    int C = kHuffCalTable[cal];         // MAX_CODE_LEN
    if (C < kHuffPureBaseCodeLen) return false;

    if (cal == kCalDefault) {
        // C == 8: all symbols at length 8
        for (int i = 0; i < kHuffPureSymbols; ++i)
            lengths[i] = kHuffPureBaseCodeLen;
        return true;
    }

    // CAL >= 3: distribute lengths 8..C evenly
    int groupCount = C - (kHuffPureBaseCodeLen - 1);  // number of distinct lengths
    int perGroup   = kHuffPureSymbols / groupCount;
    int rem        = kHuffPureSymbols % groupCount;
    int sym        = 0;
    for (int g = 0; g < groupCount; ++g) {
        uint8_t len = static_cast<uint8_t>(kHuffPureBaseCodeLen + g);
        int count   = perGroup + (g < rem ? 1 : 0);
        for (int j = 0; j < count; ++j)
            lengths[sym++] = len;
    }
    return true;
}

bool HuffGenLengthsDeflateLitLen(int cal, uint8_t lengths[kHuffLitLenSymbols]) {
    if (cal < 3 || cal > kCalMax) return false;
    int C = kHuffCalTable[cal];
    if (C < kHuffLitLenBaseCodeLen) return false;

    int groupCount = C - (kHuffLitLenBaseCodeLen - 1);  // lengths 9..C
    int perGroup   = kHuffLitLenSymbols / groupCount;
    int rem        = kHuffLitLenSymbols % groupCount;
    int sym        = 0;
    for (int g = 0; g < groupCount; ++g) {
        uint8_t len = static_cast<uint8_t>(kHuffLitLenBaseCodeLen + g);
        int count   = perGroup + (g < rem ? 1 : 0);
        for (int j = 0; j < count; ++j)
            lengths[sym++] = len;
    }
    return true;
}

void HuffGenLengthsDeflateDist(uint8_t lengths[kHuffDistSymbols]) {
    for (int i = 0; i < kHuffDistSymbols; ++i)
        lengths[i] = kHuffDistCodeLen;
}

// ============================================================================
// Canonical Huffman codeword assignment
// ============================================================================

int HuffAssignCodewords(const uint8_t lengths[], int numSymbols,
                        uint16_t codewords[], uint8_t maxLen) {
    // Collect valid symbols (length > 0 and <= maxLen)
    struct Entry {
        int sym;
        uint8_t len;
    };

    // Use a small fixed-size local array (max 286 for DEFLATE lit/len)
    Entry sorted[286];
    int n = 0;
    for (int i = 0; i < numSymbols; ++i) {
        if (lengths[i] > 0 && lengths[i] <= maxLen) {
            sorted[n].sym = i;
            sorted[n].len = lengths[i];
            ++n;
        }
    }

    // Sort by (len ascending, sym ascending) — bubble sort, small N
    for (int i = 0; i < n - 1; ++i) {
        for (int j = i + 1; j < n; ++j) {
            bool doSwap = false;
            if (sorted[i].len > sorted[j].len)
                doSwap = true;
            else if (sorted[i].len == sorted[j].len
                     && sorted[i].sym > sorted[j].sym)
                doSwap = true;
            if (doSwap) {
                Entry t = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = t;
            }
        }
    }

    if (n == 0) return 0;

    // Zero out all codewords first
    for (int i = 0; i < numSymbols; ++i)
        codewords[i] = 0;

    // Assign canonical codes (MSB-first to be written to bitstream)
    uint16_t code = 0;
    uint8_t curLen = sorted[0].len;
    for (int i = 0; i < n; ++i) {
        while (curLen < sorted[i].len) {
            code <<= 1;
            ++curLen;
        }
        codewords[sorted[i].sym] = code;
        ++code;
    }

    int maxUsed = 0;
    for (int i = 0; i < numSymbols; ++i)
        if (lengths[i] > maxUsed) maxUsed = lengths[i];
    return maxUsed;
}

// ============================================================================
// Build fast-lookup decode table (single-level, 2^maxBits entries)
// ============================================================================

void HuffBuildDecodeTable(const uint16_t codewords[],
                          const uint8_t lengths[],
                          int numSymbols,
                          uint32_t table[],
                          int maxBits) {
    int tableSize = 1 << maxBits;
    // Initialize table with sentinel value
    for (int i = 0; i < tableSize; ++i)
        table[i] = kHuffTableSentinel;

    for (int s = 0; s < numSymbols; ++s) {
        uint8_t len = lengths[s];
        if (len == 0 || len > static_cast<uint8_t>(maxBits))
            continue;

        uint16_t cw = codewords[s];
        int shift = maxBits - len;
        int base = static_cast<int>(cw) << shift;
        int stride = 1 << shift;
        for (int j = 0; j < stride; ++j) {
            int idx = base + j;
            table[idx] = (static_cast<uint32_t>(s) << 8) | len;
        }
    }
}
