//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : RLE.h
 Version     : V1.51
 By          : Wey. Silver Grid

 Description : RLE compression codec (ZRC/DPS/CPS/CPL frame types).
               Ref: 1_CSG文件格式v1.32.md §4.2

 Date        : 2026.06.26 (V1.51 — DPS cross-batch pending support)
              2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_COMPRESS_RLE_H_
#define TOOLKITS_INCLUDE_COMPRESS_RLE_H_

#include "CompressionBase.h"
#include "BitPacker.h"

class RleCodec : public CompressionBase {
public:
    RleCodec() = default;

    const char* Name() const override { return "RLE"; }
    CompressAlgorithm Algorithm() const override {
        return CompressAlgorithm::kRLE;
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

private:
    // ---- Encoder helpers ----

    /// Write a ZRC frame (transparent run)
    void WriteZRC(BitPacker& bp, int count);

    /// Write a DPS frame (discrete pixels)
    void WriteDPS(BitPacker& bp, const uint8_t* pixels, int count,
                  int crn, ColorMode cm);

    /// Write a CPS frame (short continuous run, count <= 64)
    void WriteCPS(BitPacker& bp, const uint8_t* pixel, int count,
                  int crn, ColorMode cm);

    /// Write a CPL frame (long continuous run, count <= 16384)
    void WriteCPL(BitPacker& bp, const uint8_t* pixel, int count,
                  int crn, ColorMode cm);

    /// Check if the pixel value equals the transparent color.
    bool IsTransparent(const uint8_t* pixel, int crn, ColorMode cm,
                       const uint8_t* transparentVal) const;

    // ---- Decoder helpers ----

    /// Read the transparent value (palette[0] or CRM transparent marker).
    uint8_t TransparentIndex(int crn) const;
};

#endif  // TOOLKITS_INCLUDE_COMPRESS_RLE_H_
