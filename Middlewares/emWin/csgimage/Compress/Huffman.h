//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : Huffman.h
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : Static Huffman entropy codec — self-describing header, canonical
               codewords per CSG spec §5.7.4–§5.7.5.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_COMPRESS_HUFFMAN_H_
#define TOOLKITS_INCLUDE_COMPRESS_HUFFMAN_H_

#include "CompressionBase.h"
#include <cstdint>
#include <vector>

class HuffmanCodec : public CompressionBase {
public:
    HuffmanCodec();

    const char* Name() const override { return "Huffman"; }
    CompressAlgorithm Algorithm() const override {
        return CompressAlgorithm::kHuffman;
    }

    /// Frequency-based Huffman: build tree from data, self-describing output.
    /// Output matches VP format: [origCount:4][maxLen:1][counts:maxLen*2]
    ///                          [symbol table][bsLen:4][MSB-first bitstream]
    std::vector<uint8_t> Compress(
        const uint8_t* pixels,
        int width,
        int height,
        int crn,
        ColorMode cm) override;

    /// Decode self-describing Huffman format, return decoded bytes.
    /// The output size is determined from the self-describing header (origCount).
    CSG_ErrCode Decompress(
        const uint8_t* input,
        size_t inputLen,
        uint8_t* output,
        int width,
        int height,
        int crn,
        ColorMode cm) override;

    /// Convenience: decode into a vector (for pipeline use).
    std::vector<uint8_t> DecompressVec(const uint8_t* input, size_t inputLen);

    /// Raw byte Huffman encode/decode (no pixel layout assumptions).
    /// Used by DEFLATE pipeline: LZ77 output bytes → Huffman encode.
    static std::vector<uint8_t> CompressRaw(const uint8_t* data, int count);
    static std::vector<uint8_t> DecompressRaw(const uint8_t* data, size_t len);

    // ---- DEFLATE-specific interfaces ----

    /// Compress a stream of literal / length symbols (0..285, needs uint16_t).
    std::vector<uint8_t> CompressLitLen(const uint16_t* symbols, int count);

    /// Compress a stream of distance symbols (0..31).
    std::vector<uint8_t> CompressDist(const uint8_t* symbols, int count);

    /// Decompress literal / length symbols.
    CSG_ErrCode DecompressLitLen(const uint8_t* input, size_t inputLen,
                                 uint16_t* symbols, int maxSymbols,
                                 int* outCount);

    /// Decompress distance symbols.
    CSG_ErrCode DecompressDist(const uint8_t* input, size_t inputLen,
                                uint8_t* symbols, int maxSymbols,
                                int* outCount);

    // ---- Code table accessors (for DEFLATE combined mode) ----

    const uint32_t* LitLenDecodeTable() const { return litLenDecodeTable_; }
    const uint32_t* DistDecodeTable() const { return distDecodeTable_; }
    int LitLenMaxBits() const { return litLenMaxBits_; }
    int DistMaxBits() const { return distMaxBits_; }
    const uint8_t* LitLenLengths() const { return litLenLengths_; }
    const uint8_t* DistLengths() const { return distLengths_; }

private:
    /// Build internal code tables from current CAL.
    void BuildCodeTables();

    /// Encode a symbol array into MSB-first bitstream.
    std::vector<uint8_t> EncodeSymbols(const uint8_t* symbols, int count,
                                       const uint16_t* codewords,
                                       const uint8_t* lengths);

    /// Decode symbols from an MSB-first bitstream.
    CSG_ErrCode DecodeSymbols(const uint8_t* input, size_t inputLen,
                              uint8_t* symbols, int maxSymbols,
                              int* outCount,
                              const uint16_t* decodeTable,
                              int maxBits);

    // Code lengths
    uint8_t  pureLengths_[256];
    uint8_t  litLenLengths_[286];
    uint8_t  distLengths_[32];

    // Codewords
    uint16_t pureCodewords_[256];
    uint16_t litLenCodewords_[286];
    uint16_t distCodewords_[32];

    // Decode tables (single-level, sized for worst-case CAL=7: MAX_CODE_LEN=13)
    // Size: 8192 × 2 bytes = 16 KB per table (PC encoder, acceptable).
    // MCU decoder uses multi-level tables via CSGDecoderState pointers.
    static constexpr int kHuffMaxTableBits = 13;  // CAL=7 → MAX_CODE_LEN=13
    static constexpr int kHuffMaxTableSize = 1 << kHuffMaxTableBits;  // 8192
    static constexpr int kHuffDistTableSize = 1 << kHuffDistCodeLen;  // 32

    uint32_t pureDecodeTable_[kHuffMaxTableSize];
    uint32_t litLenDecodeTable_[kHuffMaxTableSize];
    uint32_t distDecodeTable_[kHuffDistTableSize];

    int pureMaxBits_;
    int litLenMaxBits_;
    int distMaxBits_;

    bool tablesBuilt_;
};

#endif  // TOOLKITS_INCLUDE_COMPRESS_HUFFMAN_H_
