// Copyright 2026 Wey. Silver Grid. All rights reserved.
// CSG Toolkits — Compact Scalable Graphic Image Toolkit
// ---------------------------------------------------------------------------
// CSGCommon.h — Core types, enumerations, constants, CRC16, Huffman tables,
//               decoder state, and shared utility functions for CSG format.
// ---------------------------------------------------------------------------

#ifndef TOOLKITS_INCLUDE_CSGCOMMON_H_
#define TOOLKITS_INCLUDE_CSGCOMMON_H_

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <stdexcept>

// Platform detection: encoder uses Windows API, decoder is platform-agnostic.
// <windows.h> is only needed on Windows; excluded on ARM/embedded targets.
#ifdef _WIN32
// Prevent <windows.h> from defining min/max macros that break std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// ============================================================================
// Private Min/Max helpers — immune to windows.h min/max macro pollution.
// Use these instead of std::min / std::max throughout the project.
// ============================================================================

template <typename T>
inline T Min(T a, T b) noexcept { return (a < b) ? a : b; }

template <typename T>
inline T Max(T a, T b) noexcept { return (a > b) ? a : b; }

// ============================================================================
// General-purpose constants (needed by struct inline methods below)
// ============================================================================

inline constexpr int     kBitsPerByte         = 8;
inline constexpr int     kByteMask            = 0xFF;
inline constexpr uint8_t kOpaqueAlpha         = 255;
inline constexpr uint8_t kTransparentAlpha    = 0;
inline constexpr int     kRgbaChannels        = 4;   // RGBA = 4 bytes per pixel

// Header field bitmasks (used by CSGHeader / CSGPicture inline methods)
inline constexpr uint8_t kPicCountMask      = 0x7F;  // picCount bits[6:0]
inline constexpr uint8_t kPicOffsetRelFlag  = 0x80;  // picCount bit[7] = relative mode
inline constexpr uint8_t kCasMask           = 0x0F;  // Low nibble = CAS
inline constexpr uint8_t kCalMask           = 0x07;  // High nibble bits[2:0] = CAL
inline constexpr int     kCalShift          = 4;     // Shift CAL into high nibble

// ============================================================================
// Encoder identification
// ============================================================================

inline constexpr char kEncoderName[]    = "CSG Toolkits";
inline constexpr int  kEncoderVersion   = 0x01052633;  // v1.5.2633

// ============================================================================
// CSG magic markers
// ============================================================================

inline constexpr char kCsgGlobalMagic[2] = {'C', 'G'};
inline constexpr char kCsgPictureMagic[2] = {'V', 'x'};

// ============================================================================
// Version
// ============================================================================

inline constexpr uint8_t kCsgVersionMajor = 1;
inline constexpr uint8_t kCsgVersionMinor = 5;
inline constexpr uint8_t kCsgCurVersion   = (kCsgVersionMajor * 10
                                             + kCsgVersionMinor);

// ============================================================================
// Dimensional limits
// ============================================================================

inline constexpr int kCsgMaxWidth       = 256;
inline constexpr int kCsgMaxHeight      = 256;
inline constexpr int kCsgMaxPicCount    = 100;    // Max pictures per atlas
inline constexpr int kCsgMaxSinglePic   = 65500;  // Max encoded bytes per picture
inline constexpr int kCsgMaxPicOffset   = 65535;  // Max offset value (uint16_t)
inline constexpr int kCsgAlignBytes     = 4;      // Mandatory 4-byte alignment

// ============================================================================
// Compile-time compression feature switches
// Set to 0 to strip the corresponding code at compile time.
// ============================================================================

#define CAS_EN_NODE      (1)  // None (no compression)
#define CAS_EN_RLE       (1)  // RLE compression
#define CAS_EN_DEFLATE   (1)  // DEFLATE (MiniLZ77 + Huffman)
#define CAS_EN_MINILZ77  (1)  // MiniLZ77 standalone
#define CAS_EN_HUFFMAN   (1)  // Huffman standalone

#if CAS_EN_MINILZ77 == 0 || CAS_EN_HUFFMAN == 0
static_assert(CAS_EN_DEFLATE == 0,
              "DEFLATE requires both MiniLZ77 and Huffman enabled");
#endif

#ifndef ID_CSG
#define ID_CSG  0x5650
#endif

// ============================================================================
// Forward declarations
// ============================================================================

struct CSGHeader;
struct CSGPicture;
struct VxARGB;

// ============================================================================
// Color mode enumeration
// ============================================================================

enum class ColorMode : uint8_t {
    kRGB565  = 0x21,
    kBGR565  = 0x22,
    kRGB666  = 0x31,
    kRGB888  = 0x33,
    kRGB8888 = 0x41
};

// ============================================================================
// Compression algorithm enumeration (CAS)
// ============================================================================

enum class CompressAlgorithm : uint8_t {
    kNone     = 0,
    kRLE      = 1,
    kDEFLATE  = 2,
    kMiniLZ77 = 3,
    kHuffman  = 4
};

// ============================================================================
// CSG error code enumeration
// ============================================================================

enum class CSG_ErrCode : int {
    // ---- 0: Success ----
    kOk = 0,

    // ---- 1..9: Global file-layer errors ----
    kErrFileOpen        = 1,
    kErrFileRead        = 2,
    kErrFileWrite       = 3,
    kErrFileEof         = 4,
    kErrGlobalMagic     = 5,
    kErrGlobalCrc       = 6,
    kErrVersionInvalid  = 7,
    kErrPicCountZero    = 8,
    kErrOffsetTableBroken = 9,

    // ---- 10..199: Single-picture header errors ----
    kErrPicMagic        = 10,
    kErrPicCrc          = 11,
    kErrPicSizeOverflow = 12,
    kErrWidthInvalid    = 13,
    kErrHeightInvalid   = 14,
    kErrCrmInvalid      = 15,
    kErrCrnInvalid      = 16,
    kErrCasInvalid      = 17,
    kErrPposAlign       = 18,
    kErrDposAlign       = 19,
    kErrDposOverflow    = 20,
    kErrSinglePicTooLarge = 21,
    kErrPicCountLimit   = 22,

    // ---- 200..299: Palette errors ----
    kErrPaletteMiss     = 200,
    kErrPaletteLenErr   = 201,
    kErrPaletteRead     = 202,
    kErrTransparentColorInvalid = 203,

    // ---- 300..399: Compressed stream errors ----
    kErrCompStreamEmpty = 300,
    kErrRleFrameErr     = 301,
    kErrRleCriMiss      = 302,
    kErrHuffHeader      = 303,
    kErrHuffNoMatch     = 304,
    kErrLz77GroupErr    = 305,
    kErrLz77DistErr     = 306,
    kErrBitpackTrunc    = 307,
    kErrLz77CalMiss     = 308,
    kErrRleCplTrunc     = 309,

    // ---- 400..499: Memory allocation errors ----
    kErrMallocFailed    = 400,
    kErrBufferShort     = 401,

    // ---- 500..599: Encoder-specific input errors ----
    kErrSrcImgNull      = 500,
    kErrQuantFail       = 501,
    kErrCalUndefined    = 502,
    kErrExportCppFail   = 503,
    kErrImgDamaged      = 504,
    kErrProjectXmlBroken = 505,

    // ---- 600..699: Decoder runtime errors ----
    kErrStreamLimit     = 600,
    kErrOutOfRange      = 601,
    kErrExtensionBitUnknown = 602,

    // Reserved range start
    kErrReservedStart   = 1000
};

// ============================================================================
// VxARGB — in-memory 32-bit pixel representation (Little-Endian layout)
// ============================================================================

struct VxARGB {
    uint8_t b, g, r, a;

    VxARGB() : b(0), g(0), r(0), a(kOpaqueAlpha) {}
    VxARGB(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = kOpaqueAlpha)
        : b(b_), g(g_), r(r_), a(a_) {}

    bool operator==(const VxARGB& o) const noexcept {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    bool operator!=(const VxARGB& o) const noexcept {
        return !(*this == o);
    }
};

// ============================================================================
// CSGHeader — Global file header (1-byte aligned)
// ============================================================================

#pragma pack(push, 1)
struct CSGHeader {
    char     magic[2];       // "CG"
    uint16_t crc16;          // Global CRC16
    uint8_t  version;        // kCsgCurVersion
    uint8_t  picCount;       // bit[6:0] = count, bit[7] = offset mode
    uint16_t picOffset[1];   // Variable-length offset array

    CSGHeader() {
        magic[0]  = kCsgGlobalMagic[0];
        magic[1]  = kCsgGlobalMagic[1];
        version   = kCsgCurVersion;
        crc16     = 0;
        picCount  = 0;
        picOffset[0] = 0;
    }

    // Return the actual picture count (masking out the offset-mode flag).
    size_t GetPicCount() const {
        return (picCount & kPicCountMask);
    }

    // Resolve the byte offset of picture picIndex (0-based).
    // Returns 0 on error.
    uint32_t GetPicOffset(int picIndex) const {
        if (picIndex < 0
            || static_cast<size_t>(picIndex) >= GetPicCount())
            return 0;

        uint32_t result;
        if (0 == (picCount & kPicOffsetRelFlag)) {
            // Absolute offset mode
            result = picOffset[picIndex];
        } else {
            // Relative offset mode — accumulate
            result = picOffset[0];
            for (int i = 1; i <= picIndex; ++i)
                result += picOffset[i];

            if (result > kCsgMaxPicOffset)
                result = 0;
        }
        return result;
    }
};
#pragma pack(pop)

// ============================================================================
// CSGPicture — Per-picture header (1-byte aligned)
// ============================================================================

#pragma pack(push, 1)
struct CSGPicture {
    char     magic[2];       // "Vx"
    uint16_t crc16;          // Picture CRC16
    uint16_t size;           // Total picture bytes (with alignment padding)
    uint16_t width;          // Image width
    uint16_t height;         // Image height
    uint8_t  colorMode;      // ColorMode enum value
    uint8_t  crn;            // Color number
    uint8_t  compress;       // Hi nibble = CAL, Lo nibble = CAS
    uint8_t  ppos;           // Palette offset (relative)
    uint16_t dpos;           // Data offset (relative)

    CSGPicture() {
        magic[0]  = kCsgPictureMagic[0];
        magic[1]  = kCsgPictureMagic[1];
        crc16     = 0;
        size      = 0;
        width     = 0;
        height    = 0;
        colorMode = static_cast<uint8_t>(ColorMode::kRGB565);
        crn       = 0;
        ppos      = 0;
        dpos      = 0;
        compress  = 0;  // CAS=None, CAL=0
    }

    // ---- Convenience accessors for CAS/CAL nibbles ----

    CompressAlgorithm GetCompressAlgorithm() const noexcept {
        return static_cast<CompressAlgorithm>(compress & kCasMask);
    }

    uint8_t GetCompressLevel() const noexcept {
        return (compress >> kCalShift) & kCalMask;
    }

    void SetCompressAlgorithm(CompressAlgorithm algo,
                              uint8_t level = 0) noexcept {
        compress = static_cast<uint8_t>(
            ((level & kCalMask) << kCalShift) | (static_cast<uint8_t>(algo) & kCasMask));
    }

    // Derived sizes
    int BytesPerColor() const noexcept;
    int BitsPerIndex() const noexcept;
    int PaletteByteCount() const noexcept;
    int PixelCount() const noexcept { return width * height; }
};
#pragma pack(pop)

// ============================================================================
// CSGDecoderState — Streaming decoder context
// Ref: 1_CSG文件格式v1.32.md §5.7.7
// ============================================================================

// Streaming decoder window size.  Covers images up to CSG_MAX_WIDTH × N,
// where N = kDecWindowBytes / CSG_MAX_WIDTH rows.  Adjust for your MCU.
#ifndef CSG_DEC_WINDOW_BYTES
#define CSG_DEC_WINDOW_BYTES  8192   // 8 KB (covers 78×78 CRN=16 images; max lookback=128px)
#endif

struct CSGDecoderState {
    // ---- Set by caller before csgDecodePixels ----
    const uint8_t* stream;         // Compressed data pointer (in Flash)
    size_t         streamSize;     // Compressed data byte count (prevents over-read)
    uint8_t*       lineBuf;        // Output line buffer (caller-provided, ≥ width × bpc)

    // ---- Fixed image context (set by csgDecodeInit) ----
    int            width;
    int            height;
    int            pixelsDecoded;  // Cumulative decoded pixel count
    int            bpc;            // Bytes per color (CRM encoding, e.g. 2 for RGB565)
    const uint8_t* palette;        // Pointer to CRM-encoded palette (in Flash)
    int            crn;            // Color number (0 = true color, no palette lookup)
    int            bpi;            // Bits per index (0 if crn==0)

    // ---- Compression mode (set by csgDecodeInit) ----
    int            cas;            // CompressAlgorithm value (0=None,1=RLE,2=DEFLATE,3=MiniLZ77,4=Huffman)

    // ---- Internal state (do not modify) ----
    uint32_t       bitBuf;         // LZ77: current ctrl byte / bit accumulator
    int            bitCount;       // LZ77: token index (0..7) or 8=need new ctrl
    int            windowPos;      // LZ77: byte offset in compressed stream
    uint8_t        cal;            // Compression level

    // ---- Embedded sliding window (no caller allocation needed) ----
    uint8_t        window[CSG_DEC_WINDOW_BYTES];
    int            windowSize;     // Actually used bytes (≤ CSG_DEC_WINDOW_BYTES)

    // ---- Line buffer state ----
    int            linePos;        // Pixel offset within current line
    int            rowCount;       // Decoded row count

    // ---- Decoded output (CRM-encoded) ----
    // Transparent color: state.transparent[0..bpc-1] when crn>0,
    // or palette[0] when crn==0.
    uint8_t        transparent[4]; // CRM bytes of transparent color

    // ---- Pending MiniLZ77 match (carried across batch boundaries) ----
    int            pendMatchLen;  // remaining match pixels (>0 = pending)
    int            pendRef;       // source ref offset in window[]

    // ---- Pending RLE frame (carried across batch boundaries) ----
    int            pendRleCount;  // remaining pixels in current RLE frame
    int            pendRleType;   // frame type (0=ZRC, 2=CPS, 3=CPL) or -1=none
    uint8_t        pendRleCri;    // CRI value for CPS/CPL (only if pendRleType>=2)

    // ---- DEFLATE intermediate buffer handle (GUI_ALLOC, 0=none) ----
    size_t         deflateBufHandle;  // GUI_ALLOC handle for Huffman-decoded raw bytes

    // ---- Huffman state (CAS=2 DEFLATE / CAS=4 Huffman) ----
    struct { const void* table; int maxBits; } huffLit;
    struct { const void* table; int maxBits; } huffDist;

    CSGDecoderState()
        : stream(nullptr), lineBuf(nullptr),
          width(0), height(0), pixelsDecoded(0),
          bpc(0), palette(nullptr), crn(0), bpi(0),
          cas(3),  // default to MiniLZ77
          bitBuf(0), bitCount(8),
          windowPos(0), cal(0), windowSize(CSG_DEC_WINDOW_BYTES),
          linePos(0), rowCount(0) {
        transparent[0]=transparent[1]=transparent[2]=transparent[3]=0;
        pendMatchLen = 0; pendRef = 0;
        pendRleCount = 0; pendRleType = -1; pendRleCri = 0;
        deflateBufHandle = 0;
        huffLit.table = nullptr; huffLit.maxBits = 0;
        huffDist.table = nullptr; huffDist.maxBits = 0;
    }
};

// ============================================================================
// MiniLZ77 configuration per CAL level (VP-style format)
//
// Window size controls MCU SRAM for the sliding window.
// match=258 and 8-bit encLen/encDist are fixed by the format.
// ============================================================================

struct CSG_LZ77_Cfg {
    uint16_t max_window;   // Max look-back distance (encode caps window to this)
    uint16_t max_match;    // Max match length (always 258)
    uint16_t sr_win;       // Decoder window SRAM budget (bytes)
};

inline constexpr CSG_LZ77_Cfg kLz77CalTable[8] = {
    {  64, 258,   64 },  // CAL 0 — ~250 B SRAM
    { 128, 258,  128 },  // CAL 1 — ~400 B SRAM
    { 256, 258,  256 },  // CAL 2 — ~550 B SRAM (default)
    { 512, 258,  512 },  // CAL 3 — ~950 B SRAM
    {1024, 258, 1024 },  // CAL 4 — ~1.8 KB SRAM
    {2048, 258, 2048 },  // CAL 5 — ~3.5 KB SRAM
    {4096, 258, 4096 },  // CAL 6 — ~6.5 KB SRAM
    {8192, 258, 8192 },  // CAL 7 — ~12 KB SRAM (PC)
};

// ============================================================================
// Huffman MAX_CODE_LEN table per CAL
// ============================================================================

inline constexpr uint8_t kHuffCalTable[8] = {
    6, 7, 8, 9, 10, 11, 12, 13
};

// ============================================================================
// Huffman alphabet sizes (spec §5.7.4)
// ============================================================================

inline constexpr int kHuffPureSymbols   = 256;  // Pure Huffman: pixel bytes 0..255
inline constexpr int kHuffLitLenSymbols = 286;  // DEFLATE lit/len: 0..285
inline constexpr int kHuffDistSymbols   = 32;   // DEFLATE distance: 0..31

inline constexpr int kHuffLiteralMax    = 255;  // Literal symbols 0..255
inline constexpr int kHuffLenSymBase    = 257;  // First length symbol
inline constexpr int kHuffLenSymMax     = 285;  // Last length symbol
inline constexpr int kHuffDistCodeLen   = 5;    // Distance code length (always 5)
inline constexpr int kHuffPureBaseCodeLen = 8;  // Base code length for pure Huffman
inline constexpr int kHuffLitLenBaseCodeLen = 9; // Base code length for DEFLATE lit/len

// Huffman decode-table sentinel (marks unused table slots)
inline constexpr uint16_t kHuffTableSentinel = 0xFFFF;

// ============================================================================
// DEFLATE pipeline caps (Huffman alphabet limits for MiniLZ77 output)
// ============================================================================

inline constexpr int kDeflateMaxMatch  = 31;  // Lit/len symbols 257..285 → lengths 3..31
inline constexpr int kDeflateMaxWindow = 32;  // Distance symbols 0..31

// ============================================================================
// MiniLZ77 constants (spec §5.6.3, §5.6.5)
// ============================================================================

inline constexpr uint8_t kMinMatch    = 3;    // Minimum match length
inline constexpr int     kLz77HashTableSize = 65536;  // Hash buckets
inline constexpr int     kLz77HashMultiplier = 31;    // Hash function multiplier
inline constexpr int     kLz77ChainLimitStandalone = 64;   // Standalone mode
inline constexpr int     kLz77ChainLimitDeflate   = 128;  // DEFLATE mode (more thorough)

// ============================================================================
// RLE frame constants (spec §4.2)
// ============================================================================

inline constexpr int kRleFrameTypeMask  = 0x03;  // Low 2 bits = frame type
inline constexpr int kRleCountShift     = 2;     // Shift to extract count field
inline constexpr int kRleCountMask6     = 0x3F;  // 6-bit count mask
inline constexpr int kRleCplShift       = 6;     // CPL high-byte shift (14-6=8 bits)
inline constexpr int kRleZrcMax         = 63;    // ZRC max run length
inline constexpr int kRleDpsMax         = 64;    // DPS max pixel count
inline constexpr int kRleCpsMax         = 64;    // CPS max run length
inline constexpr int kRleCplMax         = 16384; // CPL max run length
inline constexpr int kRleMinRun         = 3;     // Minimum run for CPS/CPL

// ============================================================================
// CRC16 constants (poly = x¹⁶ + x¹⁵ + x² + 1, CRC-16-IBM/SDLC)
// ============================================================================

inline constexpr uint16_t kCrc16Poly     = 0x8005;
inline constexpr uint16_t kCrc16Init     = 0xFFFF;
inline constexpr uint16_t kCrc16FinalXor = 0xFFFF;

// ============================================================================
// CSG header / picture layout
// ============================================================================

inline constexpr int kCsgGlobalHeaderFixedSize = 6;    // magic[2]+crc16[2]+version[1]+picCount[1]
inline constexpr int kCsgPicOffsetEntrySize    = 2;    // Each picOffset entry is uint16_t
inline constexpr int kCsgPictureHeaderSize     = 16;   // sizeof(CSGPicture)
inline constexpr int kCppExportBytesPerLine    = 16;   // Hex bytes per line in .cpp output

// ============================================================================
// CAL
// ============================================================================

inline constexpr int     kCalTableSize = 8;   // Entries in kLz77CalTable / kHuffCalTable
inline constexpr int     kCalMax       = 7;   // Maximum valid CAL value
inline constexpr uint8_t kCalDefault   = 2;   // Default CAL level

// ============================================================================
// Valid CRN values
// ============================================================================

inline constexpr int kValidCrnValues[] = {0, 2, 4, 8, 16, 32, 64, 128};

// ============================================================================
// Color utility functions
// ============================================================================

/// Returns the number of bytes per encoded pixel for the given color mode.
inline int BytesPerColor(ColorMode cm) noexcept {
    switch (cm) {
        case ColorMode::kRGB565:  return 2;
        case ColorMode::kBGR565:  return 2;
        case ColorMode::kRGB666:  return 3;
        case ColorMode::kRGB888:  return 3;
        case ColorMode::kRGB8888: return 4;
        default:                  return 2;
    }
}

inline int CSGPicture::BytesPerColor() const noexcept {
    return ::BytesPerColor(static_cast<ColorMode>(colorMode));
}

/// Returns the number of bits needed to index CRN palette entries.
inline int BitsPerIndex(int crn) noexcept {
    switch (crn) {
        case 2:   return 1;
        case 4:   return 2;
        case 8:   return 3;
        case 16:  return 4;
        case 32:  return 5;
        case 64:  return 6;
        case 128: return 7;
        default:  return 0;
    }
}

inline int CSGPicture::BitsPerIndex() const noexcept {
    return ::BitsPerIndex(crn);
}

/// Returns the palette data length in bytes for this picture.
inline int CSGPicture::PaletteByteCount() const noexcept {
    int bpc = BytesPerColor();
    if (crn == 0) return bpc;       // Single transparent color
    return crn * bpc;
}

inline bool IsValidCrn(int crn) noexcept {
    for (int v : kValidCrnValues)
        if (v == crn) return true;
    return false;
}

inline bool IsValidColorMode(uint8_t cm) noexcept {
    switch (static_cast<ColorMode>(cm)) {
        case ColorMode::kRGB565:
        case ColorMode::kBGR565:
        case ColorMode::kRGB666:
        case ColorMode::kRGB888:
        case ColorMode::kRGB8888:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// Color packing functions
// ============================================================================

inline uint16_t ToRGB565(uint8_t r, uint8_t g, uint8_t b) noexcept {
    return static_cast<uint16_t>(
        ((r & 0xF8u) << 8) |
        ((g & 0xFCu) << 3) |
        ((b & 0xF8u) >> 3));
}

inline uint16_t ToBGR565(uint8_t r, uint8_t g, uint8_t b) noexcept {
    return static_cast<uint16_t>(
        ((b & 0xF8u) << 8) |
        ((g & 0xFCu) << 3) |
        ((r & 0xF8u) >> 3));
}

inline VxARGB FromRGB565(uint16_t val) noexcept {
    uint8_t r = static_cast<uint8_t>((val >> 8) & 0xF8);
    uint8_t g = static_cast<uint8_t>((val >> 3) & 0xFC);
    uint8_t b = static_cast<uint8_t>((val << 3) & 0xF8);
    return VxARGB(r, g, b, kOpaqueAlpha);
}

inline VxARGB FromBGR565(uint16_t val) noexcept {
    uint8_t b = static_cast<uint8_t>((val >> 8) & 0xF8);
    uint8_t g = static_cast<uint8_t>((val >> 3) & 0xFC);
    uint8_t r = static_cast<uint8_t>((val << 3) & 0xF8);
    return VxARGB(r, g, b, kOpaqueAlpha);
}

inline void ToRGB666(uint8_t r, uint8_t g, uint8_t b,
                     uint8_t out[3]) noexcept {
    out[0] = static_cast<uint8_t>((r & 0xFC) | ((g >> 4) & 0x03));
    out[1] = static_cast<uint8_t>(((g & 0x0F) << 4) | ((b >> 2) & 0x0F));
    out[2] = static_cast<uint8_t>(((b & 0x03) << 6) | ((r >> 2) & 0x3F));
}

inline VxARGB FromRGB666(const uint8_t in[3]) noexcept {
    uint8_t r = static_cast<uint8_t>(((in[0] & 0xFC) | ((in[2] << 2) & 0x03)));
    uint8_t g = static_cast<uint8_t>(((in[0] & 0x03) << 4) | ((in[1] >> 4) & 0x0F));
    uint8_t b = static_cast<uint8_t>(((in[1] & 0x0F) << 2) | ((in[2] >> 6) & 0x03));
    return VxARGB(r, g, b, kOpaqueAlpha);
}

inline void ToRGB888(uint8_t r, uint8_t g, uint8_t b,
                     uint8_t out[3]) noexcept {
    out[0] = r;
    out[1] = g;
    out[2] = b;
}

inline VxARGB FromRGB888(const uint8_t in[3]) noexcept {
    return VxARGB(in[0], in[1], in[2], kOpaqueAlpha);
}

inline void ToRGB8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                      uint8_t out[4]) noexcept {
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = a;
}

inline VxARGB FromRGB8888(const uint8_t in[4]) noexcept {
    return VxARGB(in[0], in[1], in[2], in[3]);
}

// ============================================================================
// Palette encoding / decoding helpers
// ============================================================================

// Pack a single VxARGB pixel into a color-mode byte sequence.
// The caller must provide a buffer of at least BytesPerColor(cm) bytes.
inline void PackColor(VxARGB px, ColorMode cm, uint8_t* out) noexcept {
    switch (cm) {
        case ColorMode::kRGB565: {
            uint16_t v = ToRGB565(px.r, px.g, px.b);
            out[0] = static_cast<uint8_t>(v & 0xFF);
            out[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            break;
        }
        case ColorMode::kBGR565: {
            uint16_t v = ToBGR565(px.r, px.g, px.b);
            out[0] = static_cast<uint8_t>(v & 0xFF);
            out[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            break;
        }
        case ColorMode::kRGB666:
            ToRGB666(px.r, px.g, px.b, out);
            break;
        case ColorMode::kRGB888:
            ToRGB888(px.r, px.g, px.b, out);
            break;
        case ColorMode::kRGB8888:
            ToRGB8888(px.r, px.g, px.b, px.a, out);
            break;
    }
}

// Unpack a color-mode byte sequence back to VxARGB.
inline VxARGB UnpackColor(const uint8_t* in, ColorMode cm) noexcept {
    switch (cm) {
        case ColorMode::kRGB565:
            return FromRGB565(in[0] | (static_cast<uint16_t>(in[1]) << 8));
        case ColorMode::kBGR565:
            return FromBGR565(in[0] | (static_cast<uint16_t>(in[1]) << 8));
        case ColorMode::kRGB666:
            return FromRGB666(in);
        case ColorMode::kRGB888:
            return FromRGB888(in);
        case ColorMode::kRGB8888:
            return FromRGB8888(in);
        default:
            return VxARGB();
    }
}

// ============================================================================
// Perceptual color distance (weighted)
//   R weight² = 2, G weight² = 4, B weight² = 3
// ============================================================================

inline float PerceptualDist(const VxARGB& a, const VxARGB& b) noexcept {
    float dr = static_cast<float>(a.r) - static_cast<float>(b.r);
    float dg = static_cast<float>(a.g) - static_cast<float>(b.g);
    float db = static_cast<float>(a.b) - static_cast<float>(b.b);
    return std::sqrt(2.0f * dr * dr + 4.0f * dg * dg + 3.0f * db * db);
}

inline float PerceptualDistSq(const VxARGB& a, const VxARGB& b) noexcept {
    float dr = static_cast<float>(a.r) - static_cast<float>(b.r);
    float dg = static_cast<float>(a.g) - static_cast<float>(b.g);
    float db = static_cast<float>(a.b) - static_cast<float>(b.b);
    return 2.0f * dr * dr + 4.0f * dg * dg + 3.0f * db * db;
}

// ============================================================================
// CRC16 — polynomial x¹⁶ + x¹⁵ + x² + 1 (CRC-16-IBM/SDLC, poly=0x8005)
// Initial: 0xFFFF, Final XOR: 0xFFFF (ones' complement)
// ============================================================================

uint16_t Crc16(const void* data, uint32_t numBytes) noexcept;

// ============================================================================
// Color mode / compression algorithm name helpers
// ============================================================================

const std::string& ColorModeName(ColorMode cm);
const std::string& CompressModeName(CompressAlgorithm algo);

// ============================================================================
// Error code to string
// ============================================================================

const char* CsgGetErrStr(CSG_ErrCode code) noexcept;

// ============================================================================
// Current time string (for .cpp/.h generation)
// ============================================================================

std::string CurrentTimeString();

// ============================================================================
// Wide-string to UTF-8 conversion
// ============================================================================

std::string WideToUtf8(const std::wstring& ws);

// ============================================================================
// File I/O utilities
// ============================================================================

void WriteBinaryFile(const std::wstring& path,
                     const std::vector<uint8_t>& bytes);
void WriteTextFile(const std::wstring& path, const std::string& text);

// ============================================================================
// Caption / file-header generation for .cpp output
// ============================================================================

std::string GenerateCppCaption(const std::string& symbolName);
std::string GenerateHeaderGuard(const std::string& fileName,
                                const std::string& symbolName,
                                const std::vector<std::string>& picNames);

// ============================================================================
// Huffman code length generation
// Ref: 1_CSG文件格式v1.32.md §5.7.4
// ============================================================================

/// Generate code lengths for pure Huffman mode (256 symbols).
/// CAL must be >= 2 per spec.
bool HuffGenLengthsPure(int cal, uint8_t lengths[256]);

/// Generate code lengths for DEFLATE literal/length table (286 symbols).
/// CAL must be >= 3 per spec.
bool HuffGenLengthsDeflateLitLen(int cal, uint8_t lengths[286]);

/// Generate code lengths for DEFLATE distance table (32 symbols).
/// All symbols get length 5 regardless of CAL.
void HuffGenLengthsDeflateDist(uint8_t lengths[32]);

// ============================================================================
// Canonical Huffman codeword assignment
// Ref: 1_CSG文件格式v1.32.md §5.7.5
// ============================================================================

/// Assign canonical Huffman codewords (MSB-first) from code lengths.
/// Returns the maximum code length encountered.
int HuffAssignCodewords(const uint8_t lengths[], int numSymbols,
                        uint16_t codewords[], uint8_t maxLen);

/// Build a fast-lookup decode table from codewords.
/// `table` must have 2^maxBits entries (uint32_t for symbols up to 285).
void HuffBuildDecodeTable(const uint16_t codewords[],
                          const uint8_t lengths[],
                          int numSymbols,
                          uint32_t table[],
                          int maxBits);

// ============================================================================
// Decoder API declarations
// ============================================================================

CSG_ErrCode CsgDecodeInit(CSGDecoderState* state,
                          const CSGPicture* pic,
                          uint8_t* lineBuf,
                          const uint8_t* palette,
                          ColorMode outMode);
CSG_ErrCode CsgDecodePixels(CSGDecoderState* state,
                            uint8_t* output,
                            int requestedPixels);

// ============================================================================
// Alignment helper
// ============================================================================

inline uint32_t Align4(uint32_t v) noexcept {
    return (v + 3u) & ~3u;
}

inline bool IsAligned4(uint32_t v) noexcept {
    return (v & 3u) == 0;
}

#endif  // TOOLKITS_INCLUDE_CSGCOMMON_H_
