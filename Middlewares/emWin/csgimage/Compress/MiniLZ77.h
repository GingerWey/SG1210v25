//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : MiniLZ77.h
 Version     : V1.51
 By          : Wey. Silver Grid

 Description : MiniLZ77 dictionary compression codec — VP-style 8-token control
               groups.  Lightweight LZ77 designed for MCU decoding.

 Date        : 2026.06.26 (V1.51 — circular window indexing for large images)
              2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_COMPRESS_MINILZ77_H_
#define TOOLKITS_INCLUDE_COMPRESS_MINILZ77_H_

#include "CompressionBase.h"
#include <cstdint>
#include <vector>

class MiniLZ77Codec : public CompressionBase {
public:
    MiniLZ77Codec();

    const char* Name() const override { return "MiniLZ77"; }
    CompressAlgorithm Algorithm() const override {
        return CompressAlgorithm::kMiniLZ77;
    }

    std::vector<uint8_t> Compress(
        const uint8_t* pixels,
        int width,
        int height,
        int crn,
        ColorMode cm) override;

    CSG_ErrCode Decompress(
        const uint8_t* input,
        size_t inputLen,
        uint8_t* output,
        int width,
        int height,
        int crn,
        ColorMode cm) override;

    // ---- DEFLATE symbol-stream interface ----
    // These output raw literal/length/distance symbols for downstream
    // Huffman encoding (not byte-packed).

    struct Lz77Symbol {
        bool isLiteral;         // true = literal byte, false = match pair
        uint8_t literal;        // Literal byte value (only if isLiteral)
        uint16_t distance;      // Match distance (only if !isLiteral)
        uint16_t length;        // Match length  (only if !isLiteral)
    };

    /// Compress to a sequence of LZ77 symbols for DEFLATE pipeline.
    std::vector<Lz77Symbol> CompressToSymbols(
        const uint8_t* pixels,
        int width,
        int height);

    // ---- Accessors for current CAL parameters ----
    const CSG_LZ77_Cfg& GetCfg() const;
    int LenBits() const;

private:
    // Get CAL configuration
    void UpdateCfg();

    CSG_LZ77_Cfg cfg_;
    int lenBits_;   // Length field bit-width per CAL
};

#endif  // TOOLKITS_INCLUDE_COMPRESS_MINILZ77_H_
