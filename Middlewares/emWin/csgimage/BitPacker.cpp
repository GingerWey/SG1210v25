//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : BitPacker.cpp
 Version     : V1.50
 By          : Wey. Silver Grid

 Description : BitPacker implementation — LSB-first bit-stream pack/unpack.

 Date        : 2026.06.25 (V1.50 — original CSG v1.5 implementation)
*/
//-----------------------------------------------------------------------------
#include "BitPacker.h"

// ============================================================================
// BitPacker (encoder side)
// ============================================================================

void BitPacker::PackBits(uint8_t value, int numBits) {
    for (int i = 0; i < numBits; ++i) {
        if (value & (1u << i)) {
            currentByte_ |= static_cast<uint8_t>(1u << bitOffset_);
        }
        ++bitOffset_;
        if (bitOffset_ == 8) {
            buf_.push_back(currentByte_);
            currentByte_ = 0;
            bitOffset_ = 0;
        }
    }
}

void BitPacker::PackBits16(uint16_t value, int numBits) {
    for (int i = 0; i < numBits; ++i) {
        if (value & (1u << i)) {
            currentByte_ |= static_cast<uint8_t>(1u << bitOffset_);
        }
        ++bitOffset_;
        if (bitOffset_ == 8) {
            buf_.push_back(currentByte_);
            currentByte_ = 0;
            bitOffset_ = 0;
        }
    }
}

std::vector<uint8_t> BitPacker::GetBytes() const {
    std::vector<uint8_t> out = buf_;
    if (bitOffset_ > 0) {
        // Partial byte — zero-padded on the high side
        out.push_back(currentByte_);
    }
    return out;
}

void BitPacker::Reset() {
    buf_.clear();
    currentByte_ = 0;
    bitOffset_ = 0;
}

// ============================================================================
// BitUnpacker (decoder side)
// ============================================================================

uint8_t BitUnpacker::UnpackBits(int numBits) {
    uint8_t result = 0;
    for (int i = 0; i < numBits; ++i) {
        size_t byteIdx = bitPos_ / 8;
        size_t bitInByte = bitPos_ % 8;
        if (byteIdx < byteLen_) {
            if (data_[byteIdx] & (1u << bitInByte)) {
                result |= static_cast<uint8_t>(1u << i);
            }
        }
        ++bitPos_;
    }
    return result;
}

uint16_t BitUnpacker::UnpackBits16(int numBits) {
    uint16_t result = 0;
    for (int i = 0; i < numBits; ++i) {
        size_t byteIdx = bitPos_ / 8;
        size_t bitInByte = bitPos_ % 8;
        if (byteIdx < byteLen_) {
            if (data_[byteIdx] & (1u << bitInByte)) {
                result |= static_cast<uint16_t>(1u << i);
            }
        }
        ++bitPos_;
    }
    return result;
}

uint8_t BitUnpacker::PeekBits(int numBits) const {
    // Read bits without advancing: compute from current position inline.
    uint8_t result = 0;
    size_t pos = bitPos_;
    for (int i = 0; i < numBits; ++i) {
        size_t byteIdx = pos / 8;
        size_t bitInByte = pos % 8;
        if (byteIdx < byteLen_) {
            if (data_[byteIdx] & (1u << bitInByte)) {
                result |= static_cast<uint8_t>(1u << i);
            }
        }
        ++pos;
    }
    return result;
}
