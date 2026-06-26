// Copyright 2026 Wey. Silver Grid. All rights reserved.
// CSG Toolkits — Core Codec Utilities
// ---------------------------------------------------------------------------
// CSGCodec.h — Shared encode/decode infrastructure:
//              - CSG file header read / write
//              - Picture header read / write
//              - Alignment padding helpers
//              - CRC computation over structured data
//              - .csg binary file read / write
//              - .cpp/.h source file export
// ---------------------------------------------------------------------------

#ifndef TOOLKITS_INCLUDE_CSGCODEC_H_
#define TOOLKITS_INCLUDE_CSGCODEC_H_

#include "CSGCommon.h"
#include "Compress/CompressionBase.h"
#include <vector>
#include <memory>
#include <string>
#include <map>

// ============================================================================
// Forward declarations
// ============================================================================

class RleCodec;
class HuffmanCodec;
class MiniLZ77Codec;

// ============================================================================
// CSGPictureEncodeData — in-memory representation of one picture being encoded
// ============================================================================

struct CSGPictureEncodeData {
    // Original image info
    std::string         imageFileName;
    std::string         pictureName;
    int                 srcWidth  = 0;
    int                 srcHeight = 0;
    int                 srcFileSize = 0;

    // Target CSG parameters
    int                 targetWidth  = 0;
    int                 targetHeight = 0;
    ColorMode           colorMode = ColorMode::kRGB565;
    int                 crn = 0;
    CompressAlgorithm   algo = CompressAlgorithm::kNone;
    uint8_t             cal = 2;

    // Palette data
    std::vector<uint8_t> palette;     // CRM-encoded palette bytes

    // Raw pixel data (CRM-encoded for CRN=0, or palette indices for CRN>0)
    std::vector<uint8_t> rawPixelData;

    // Compressed pixel data
    std::vector<uint8_t> compressedData;

    // Encoded picture bytes (header + palette + compressed data + padding)
    std::vector<uint8_t> encodedPicture;
    uint16_t             encodedSize = 0;

    // CRC values
    uint16_t             pictureCrc16 = 0;
};

// ============================================================================
// CSGAtlas — an in-memory CSG atlas (multiple pictures)
// ============================================================================

struct CSGAtlas {
    std::string                    atlasName;
    std::vector<CSGPictureEncodeData> pictures;
    uint16_t                       globalCrc16 = 0;

    // Serialized bytes
    std::vector<uint8_t>           bytes;

    bool IsValid() const {
        return !pictures.empty()
            && pictures.size() <= static_cast<size_t>(kCsgMaxPicCount);
    }

    int PictureCount() const {
        return static_cast<int>(pictures.size());
    }
};

// ============================================================================
// Codec factory
// ============================================================================

/// Create a compression codec instance for the given algorithm.
std::unique_ptr<CompressionBase> CreateCodec(CompressAlgorithm algo);

/// Create a codec and set its CAL level.
std::unique_ptr<CompressionBase> CreateCodec(CompressAlgorithm algo,
                                              uint8_t cal);

// ============================================================================
// Header I/O
// ============================================================================

/// Encode a complete CSG binary byte stream for an atlas.
/// Returns true on success; sets atlas.bytes and CRC.
bool EncodeAtlas(CSGAtlas& atlas);

/// Decode a CSG binary buffer into an atlas structure.
/// Returns error code.
CSG_ErrCode DecodeAtlasHeader(const uint8_t* data, size_t len,
                               CSGHeader& header);

/// Read picture offsets from raw CSG data into a vector.
/// Handles absolute / relative offset modes per §3.2.2-3.2.3.
/// Fixes the flexible-array limitation of CSGHeader::GetPicOffset().
/// Returns empty vector on error.
std::vector<uint32_t> ReadPicOffsets(const uint8_t* data, size_t len);

/// Decode a single picture header from a pointer (in a loaded atlas).
CSG_ErrCode DecodePictureHeader(const uint8_t* data, size_t maxLen,
                                 CSGPicture& picture);

// ============================================================================
// File I/O
// ============================================================================

/// Write a CSG atlas to a .csg binary file.
bool WriteCsgFile(const std::wstring& path, const CSGAtlas& atlas);

/// Read a CSG atlas from a .csg binary file.
CSG_ErrCode ReadCsgFile(const std::wstring& path, CSGAtlas& atlas);

// ============================================================================
// C++ source-code export
// ============================================================================

/// Export a CSG atlas as .cpp / .h source-code pair.
/// Returns true on success.
bool ExportCppSource(const std::wstring& cppPath,
                     const std::wstring& hPath,
                     const CSGAtlas& atlas,
                     const std::string& symbolName);

// ============================================================================
// Utility
// ============================================================================

/// Format bytes as comma-separated hex with newlines (16 bytes per line).
std::string FormatByteArray(const uint8_t* data, size_t len,
                            const std::string& indent = "  ");

/// Align a vector's size upward to 4 bytes by appending zeros.
void PadToAlign4(std::vector<uint8_t>& vec);

/// Compute the header size needed for a given picture count.
int GlobalHeaderSize(int pictureCount);

#endif  // TOOLKITS_INCLUDE_CSGCODEC_H_
