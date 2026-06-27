//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : CSGDecoder.h
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : CSG decoder class — DecompressData, ConvertToRGBA, DecodePicture,
               DecodeAtlasPicture, DecodeAtlas.  PC-side full-decode API.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 decoder header)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_CSGDECODER_H_
#define TOOLKITS_INCLUDE_CSGDECODER_H_

#include "CSGCommon.h"
#include "CSGCodec.h"
#include <vector>
#include <string>

/// Decoder result for a single picture.
struct DecoderResult {
    CSG_ErrCode          error = CSG_ErrCode::kOk;
    std::string          errorDetail;

    uint16_t             width = 0;
    uint16_t             height = 0;
    ColorMode            colorMode = ColorMode::kRGB565;
    int                  crn = 0;
    CompressAlgorithm    algo = CompressAlgorithm::kNone;
    uint8_t              cal = 0;

    std::vector<uint8_t> palette;        // Raw CRM-encoded palette bytes
    std::vector<uint8_t> pixels;         // Decoded RGBA pixels (4 bytes each)
};

/// Main decoder class (PC / host-side one-shot decoder).
///
/// Uses std::vector and heap allocation — appropriate for PC environments.
/// For MCU targets (STM32F4/F7), use the streaming CSGDecoderState API instead
/// (csgDecodeInit / csgDecodePixels), which uses only caller-provided static
/// buffers per CSG spec §5.7.1.
///
/// Usage (PC, one-shot):
///   CSGDecoder decoder;
///   DecoderResult result = decoder.DecodePicture(picData, picDataLen);
///
/// Usage (MCU, streaming — via CSGDecoderState):
///   CSGDecoderState state;
///   csgDecodeInit(&state, &picture, lineBuffer);
///   csgDecodePixels(&state, output, requestedPixels);
class CSGDecoder {
public:
    CSGDecoder();

    /// Decode a single CSG picture from its encoded byte buffer.
    DecoderResult DecodePicture(const uint8_t* data, size_t len);

    /// Decode a specific picture from an atlas by index (0-based).
    DecoderResult DecodeAtlasPicture(const uint8_t* atlasData,
                                     size_t atlasLen,
                                     int pictureIndex);

    /// Decode all pictures from an atlas.
    std::vector<DecoderResult> DecodeAtlas(const uint8_t* atlasData,
                                            size_t atlasLen);

private:
    /// Decompress picture data to raw CRM bytes or CRI indices.
    CSG_ErrCode DecompressData(const uint8_t* compData,
                               size_t compLen,
                               const CSGPicture& header,
                               std::vector<uint8_t>& rawOutput);

    /// Convert raw CRM/CRI data + palette to RGBA pixels.
    CSG_ErrCode ConvertToRGBA(const uint8_t* rawData,
                              const CSGPicture& header,
                              const std::vector<uint8_t>& palette,
                              std::vector<uint8_t>& rgbaOutput);
};

#endif  // TOOLKITS_INCLUDE_CSGDECODER_H_
