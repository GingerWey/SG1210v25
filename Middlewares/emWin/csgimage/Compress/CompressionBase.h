//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CompressionBase.h
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : Abstract interface for all CSG compression codecs
               (RLE, Huffman, MiniLZ77, DEFLATE).

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_COMPRESS_COMPRESSIONBASE_H_
#define TOOLKITS_INCLUDE_COMPRESS_COMPRESSIONBASE_H_

#include "CSGCommon.h"
#include <cstdint>
#include <vector>

/// Abstract base class for compression / decompression algorithms.
///
/// All codecs operate on a flat pixel buffer of `VxARGB` pixels.
/// The `cal` parameter (0..7) controls compression-aggressiveness
/// parameters as defined in Table 5-2 of the spec.
class CompressionBase {
public:
    virtual ~CompressionBase() = default;

    /// Set the compression level (0..7).
    void SetCal(uint8_t cal);

    /// Current compression level.
    uint8_t GetCal() const { return cal_; }

    /// Compress a flat array of pixels into a byte stream.
    /// - For CRN > 0, `indices` contains palette indices.
    /// - For CRN = 0 (true-color), `pixels` contains raw CRM-encoded bytes.
    /// `width` and `height` provide image dimensions used for row-aware
    /// compression strategies.
    ///
    /// Returns compressed byte stream (may be empty on failure).
    virtual std::vector<uint8_t> Compress(
        const uint8_t* pixels,
        int width,
        int height,
        int crn,
        ColorMode cm) = 0;

    /// Decompress a byte stream back into raw pixel CRM data or indices.
    /// The caller provides a pre-allocated output buffer;
    /// `outputLen` bytes are written.
    ///
    /// Returns the error code (kOk on success).
    virtual CSG_ErrCode Decompress(
        const uint8_t* input,
        size_t inputLen,
        uint8_t* output,
        int width,
        int height,
        int crn,
        ColorMode cm) = 0;

    /// Human-readable algorithm name.
    virtual const char* Name() const = 0;

    /// CAS enum value for this codec.
    virtual CompressAlgorithm Algorithm() const = 0;

protected:
    uint8_t cal_ = 2;  // Default CAL
};

#endif  // TOOLKITS_INCLUDE_COMPRESS_COMPRESSIONBASE_H_
