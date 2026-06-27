//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : BitPacker.h
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : LSB-first bit-stream packer / unpacker for CSG CRI index coding.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#ifndef TOOLKITS_INCLUDE_BITPACKER_H_
#define TOOLKITS_INCLUDE_BITPACKER_H_

#include <cstdint>
#include <vector>

/// Packs values LSB-first into a byte buffer.
///
/// Usage (pack):
///   BitPacker bp;
///   bp.PackBits(value, numBits);
///   // ... more calls ...
///   std::vector<uint8_t> bytes = bp.GetBytes();
///
/// Usage (unpack):
///   BitUnpacker bu(bytes.data(), bytes.size());
///   uint8_t val = bu.UnpackBits(numBits);
class BitPacker {
public:
    BitPacker() : currentByte_(0), bitOffset_(0) {}

    /// Pack the lowest `numBits` bits of `value`, LSB first.
    void PackBits(uint8_t value, int numBits);

    /// Pack a 16-bit value's lowest `numBits` bits.
    void PackBits16(uint16_t value, int numBits);

    /// Flush any partially-filled byte (zero-padded on high side)
    /// and return the complete buffer.
    std::vector<uint8_t> GetBytes() const;

    /// Reset to empty state.
    void Reset();

    /// Number of whole bytes written so far.
    size_t ByteCount() const { return buf_.size(); }

    /// Number of bits accumulated in the current partial byte.
    int BitOffset() const { return bitOffset_; }

private:
    std::vector<uint8_t> buf_;
    uint8_t  currentByte_;
    int      bitOffset_;    // bits already written into currentByte_
};

/// Unpacks values LSB-first from a byte buffer.
class BitUnpacker {
public:
    BitUnpacker(const uint8_t* data, size_t byteLen)
        : data_(data), byteLen_(byteLen),
          bitPos_(0) {}

    /// Read `numBits` bits (LSB first), advance position.
    uint8_t UnpackBits(int numBits);

    /// Read `numBits` bits into a 16-bit value.
    uint16_t UnpackBits16(int numBits);

    /// Peek at the next `numBits` bits without advancing.
    uint8_t PeekBits(int numBits) const;

    /// Total number of bits remaining.
    size_t BitsRemaining() const {
        size_t totalBits = byteLen_ * 8;
        return (bitPos_ < totalBits) ? (totalBits - bitPos_) : 0;
    }

    /// Current bit position.
    size_t BitPos() const { return bitPos_; }

    /// Skip `n` bits.
    void SkipBits(int n) { bitPos_ += n; }

    /// Reset to start of buffer.
    void Reset() { bitPos_ = 0; }

private:
    const uint8_t* data_;
    size_t         byteLen_;
    size_t         bitPos_;     // global bit position
};

#endif  // TOOLKITS_INCLUDE_BITPACKER_H_
