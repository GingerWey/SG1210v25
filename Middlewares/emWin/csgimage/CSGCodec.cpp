// Copyright 2026 Wey. Silver Grid. All rights reserved.
// CSG Toolkits — Core Codec implementation
// ---------------------------------------------------------------------------

#include "CSGCodec.h"
#include "Compress/RLE.h"
#include "Compress/Huffman.h"
#include "Compress/MiniLZ77.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// ============================================================================
// Codec factory
// ============================================================================

std::unique_ptr<CompressionBase> CreateCodec(CompressAlgorithm algo) {
    switch (algo) {
        case CompressAlgorithm::kRLE:
            return std::make_unique<RleCodec>();
        case CompressAlgorithm::kHuffman:
            return std::make_unique<HuffmanCodec>();
        case CompressAlgorithm::kMiniLZ77:
            return std::make_unique<MiniLZ77Codec>();
        case CompressAlgorithm::kDEFLATE:
            // DEFLATE is handled as a pipeline (MiniLZ77 + Huffman)
            // Return nullptr — caller must compose the pipeline
            return nullptr;
        case CompressAlgorithm::kNone:
        default:
            return nullptr;  // No compression or unknown
    }
}

std::unique_ptr<CompressionBase> CreateCodec(CompressAlgorithm algo,
                                              uint8_t cal) {
    auto c = CreateCodec(algo);
    if (c) c->SetCal(cal);
    return c;
}

// ============================================================================
// Alignment helpers
// ============================================================================

void PadToAlign4(std::vector<uint8_t>& vec) {
    while (vec.size() % 4 != 0)
        vec.push_back(0);
}

int GlobalHeaderSize(int pictureCount) {
    // Fixed header + offset table + 4-byte alignment
    int size = kCsgGlobalHeaderFixedSize + pictureCount * kCsgPicOffsetEntrySize;
    return static_cast<int>(Align4(static_cast<uint32_t>(size)));
}

// ============================================================================
// Encode a single picture
// ============================================================================

static bool EncodeSinglePicture(CSGPictureEncodeData& pic) {
    int pixelCount = pic.targetWidth * pic.targetHeight;
    int bpc = BytesPerColor(pic.colorMode);
    int bpi = (pic.crn > 0) ? BitsPerIndex(pic.crn) : 0;

    // ---- Step 1: Raw pixel data (CRM-encoded or CRI indices) ----
    if (pic.crn > 0) {
        // Palette mode: pixel bytes are CRI indices
        // The rawPixelData should already contain palette indices
    } else {
        // True-color: pixel bytes are CRM-encoded
        // The rawPixelData should already contain CRM bytes
    }

    int rawStride = (pic.crn > 0) ? 1 : bpc;
    int expectedRawSize = pixelCount * rawStride;
    if (static_cast<int>(pic.rawPixelData.size()) != expectedRawSize) {
        // Attempt to fix: pad or truncate
        pic.rawPixelData.resize(expectedRawSize, 0);
    }

    // ---- Step 2: Compress if needed ----
    if (pic.algo == CompressAlgorithm::kNone) {
        // No compression — bit-pack palette indices, or store CRM bytes directly
        if (pic.crn > 0) {
            // Bit-pack CRI indices
            BitPacker bp;
            for (int idx : pic.rawPixelData)
                bp.PackBits(static_cast<uint8_t>(idx), bpi);
            pic.compressedData = bp.GetBytes();
        } else {
            pic.compressedData = pic.rawPixelData;
        }
    } else if (pic.algo == CompressAlgorithm::kDEFLATE) {
        // DEFLATE pipeline: MiniLZ77 → Huffman (VP-style mLZ+Huffman)
        auto lz77 = std::make_unique<MiniLZ77Codec>();
        auto lzBytes = lz77->Compress(
            pic.rawPixelData.data(),
            pic.targetWidth, pic.targetHeight,
            pic.crn, pic.colorMode);

        pic.compressedData = HuffmanCodec::CompressRaw(
            lzBytes.data(), static_cast<int>(lzBytes.size()));
    } else {
        // RLE, Huffman, or MiniLZ77 standalone
        auto codec = CreateCodec(pic.algo, pic.cal);
        if (!codec) return false;

        pic.compressedData = codec->Compress(
            pic.rawPixelData.data(),
            pic.targetWidth,
            pic.targetHeight,
            pic.crn,
            pic.colorMode);
    }

    // ---- Step 3: Build CSGPicture header ----
    CSGPicture header;
    header.width     = static_cast<uint16_t>(pic.targetWidth);
    header.height    = static_cast<uint16_t>(pic.targetHeight);
    header.colorMode = static_cast<uint8_t>(pic.colorMode);
    header.crn       = static_cast<uint8_t>(pic.crn);
    header.SetCompressAlgorithm(pic.algo, pic.cal);

    // Palette offset = sizeof(CSGPicture) = 16 bytes
    uint8_t ppos = kCsgPictureHeaderSize;
    header.ppos = ppos;

    // Data offset = ppos + palette_bytes (aligned to 4)
    uint32_t palBytes = static_cast<uint32_t>(pic.palette.size());
    uint32_t palAligned = Align4(palBytes);
    uint32_t dpos = ppos + palAligned;
    header.dpos = static_cast<uint16_t>(dpos);

    // Picture size = dpos + compressed_data_size (aligned to 4)
    uint32_t totalSize = dpos + Align4(static_cast<uint32_t>(
        pic.compressedData.size()));
    header.size = static_cast<uint16_t>(totalSize);

    // ---- Step 4: Serialize picture ----
    pic.encodedPicture.clear();

    // Write header (16 bytes)
    const auto* headerBytes = reinterpret_cast<const uint8_t*>(&header);
    pic.encodedPicture.insert(pic.encodedPicture.end(),
                              headerBytes, headerBytes + kCsgPictureHeaderSize);

    // Write palette
    pic.encodedPicture.insert(pic.encodedPicture.end(),
                              pic.palette.begin(), pic.palette.end());

    // Pad after palette to 4-byte alignment
    while (pic.encodedPicture.size() % 4 != 0)
        pic.encodedPicture.push_back(0);

    // Write compressed data
    pic.encodedPicture.insert(pic.encodedPicture.end(),
                              pic.compressedData.begin(),
                              pic.compressedData.end());

    // Pad end to 4-byte alignment
    while (pic.encodedPicture.size() % 4 != 0)
        pic.encodedPicture.push_back(0);

    pic.encodedSize = static_cast<uint16_t>(pic.encodedPicture.size());

    // ---- Step 5: Compute picture CRC16 ----
    // CRC covers bytes 4 through end of picture (all padding included)
    pic.pictureCrc16 = Crc16(pic.encodedPicture.data() + 4,
                              static_cast<uint32_t>(pic.encodedPicture.size() - 4));

    // Update size/ppos/dpos/crc16 in the serialized header via memcpy
    // to avoid unaligned-access faults on ARM (vector<uint8_t> has 1-byte
    // alignment but CSGPicture has uint16_t members at offsets 2,4,6,8,14).
    {
        CSGPicture hdrCopy;
        std::memcpy(&hdrCopy, pic.encodedPicture.data(), sizeof(CSGPicture));
        hdrCopy.size  = pic.encodedSize;
        hdrCopy.ppos  = ppos;
        hdrCopy.dpos  = static_cast<uint16_t>(dpos);
        hdrCopy.crc16 = pic.pictureCrc16;
        std::memcpy(pic.encodedPicture.data(), &hdrCopy, sizeof(CSGPicture));
    }

    return true;
}

// ============================================================================
// Encode a complete atlas
// ============================================================================

bool EncodeAtlas(CSGAtlas& atlas) {
    if (!atlas.IsValid()) return false;

    // Encode each picture first
    for (auto& pic : atlas.pictures) {
        if (!EncodeSinglePicture(pic))
            return false;
    }

    // ---- Build global header ----
    int picCount = atlas.PictureCount();
    int headerSize = GlobalHeaderSize(picCount);

    // Collect absolute offsets
    std::vector<uint32_t> absOffsets(picCount);
    uint32_t currentOffset = static_cast<uint32_t>(headerSize);
    for (int i = 0; i < picCount; ++i) {
        absOffsets[i] = currentOffset;
        currentOffset += Align4(static_cast<uint32_t>(
            atlas.pictures[i].encodedPicture.size()));
    }

    // Determine offset mode
    bool useRelative = (absOffsets.back() >= kCsgMaxPicOffset);

    // Build header directly into the byte buffer to avoid writing past
    // CSGHeader::picOffset[1] (the struct only declares one array element).
    // Layout: magic[2] + crc16[2] + version[1] + picCount[1] = 6 bytes
    //         + picOffset[picCount * 2] + zero-padding to 4-byte alignment.
    std::vector<uint8_t> headerBytes(headerSize, 0);

    // Fixed 6-byte portion
    headerBytes[0] = static_cast<uint8_t>(kCsgGlobalMagic[0]);
    headerBytes[1] = static_cast<uint8_t>(kCsgGlobalMagic[1]);
    headerBytes[2] = 0;  // crc16 low  (filled after CRC computation)
    headerBytes[3] = 0;  // crc16 high
    headerBytes[4] = kCsgCurVersion;

    uint8_t countByte = static_cast<uint8_t>(picCount & 0x7F);
    if (useRelative)
        countByte |= 0x80;
    headerBytes[5] = countByte;

    // Variable-length picOffset array at offset 6
    if (useRelative) {
        uint16_t v = static_cast<uint16_t>(absOffsets[0]);
        headerBytes[6] = static_cast<uint8_t>(v & 0xFF);
        headerBytes[7] = static_cast<uint8_t>((v >> 8) & 0xFF);
        for (int i = 1; i < picCount; ++i) {
            uint32_t rel = absOffsets[i] - absOffsets[i - 1];
            uint16_t rv = static_cast<uint16_t>(rel);
            headerBytes[6 + i * 2]     = static_cast<uint8_t>(rv & 0xFF);
            headerBytes[6 + i * 2 + 1] = static_cast<uint8_t>((rv >> 8) & 0xFF);
        }
    } else {
        for (int i = 0; i < picCount; ++i) {
            uint16_t v = static_cast<uint16_t>(absOffsets[i]);
            headerBytes[6 + i * 2]     = static_cast<uint8_t>(v & 0xFF);
            headerBytes[6 + i * 2 + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        }
    }

    // ---- Concatenate everything ----
    atlas.bytes.clear();
    atlas.bytes.insert(atlas.bytes.end(),
                       headerBytes.begin(), headerBytes.end());

    for (int i = 0; i < picCount; ++i) {
        const auto& picBytes = atlas.pictures[i].encodedPicture;
        atlas.bytes.insert(atlas.bytes.end(),
                           picBytes.begin(), picBytes.end());
        // Pad to 4-byte alignment between pictures
        while (atlas.bytes.size() % 4 != 0)
            atlas.bytes.push_back(0);
    }

    // ---- Compute global CRC16 ----
    // CRC covers bytes 4 through end of file (all padding included)
    atlas.globalCrc16 = Crc16(atlas.bytes.data() + 4,
                               static_cast<uint32_t>(atlas.bytes.size() - 4));

    // Write CRC into header (bytes 2-3) via memcpy for ARM alignment safety
    {
        CSGHeader hdrCopy;
        std::memcpy(&hdrCopy, atlas.bytes.data(), sizeof(CSGHeader));
        hdrCopy.crc16 = atlas.globalCrc16;
        std::memcpy(atlas.bytes.data(), &hdrCopy, sizeof(CSGHeader));
    }

    return true;
}

// ============================================================================
// Decode header helpers
// ============================================================================

CSG_ErrCode DecodeAtlasHeader(const uint8_t* data, size_t len,
                               CSGHeader& header) {
    // Need at least fixed 6 bytes + 2 bytes for first picOffset
    if (len < sizeof(CSGHeader))
        return CSG_ErrCode::kErrFileEof;

    std::memcpy(&header, data, sizeof(CSGHeader));

    // Verify magic
    if (header.magic[0] != kCsgGlobalMagic[0]
        || header.magic[1] != kCsgGlobalMagic[1])
        return CSG_ErrCode::kErrGlobalMagic;

    // Verify version
    if (header.version > kCsgCurVersion)
        return CSG_ErrCode::kErrVersionInvalid;

    // Verify picture count
    int count = static_cast<int>(header.GetPicCount());
    if (count == 0)
        return CSG_ErrCode::kErrPicCountZero;
    if (count > kCsgMaxPicCount)
        return CSG_ErrCode::kErrPicCountLimit;

    // Verify global CRC
    if (header.crc16 != 0) {
        uint16_t computed = Crc16(data + 4, static_cast<uint32_t>(len - 4));
        if (computed != header.crc16)
            return CSG_ErrCode::kErrGlobalCrc;
    }

    return CSG_ErrCode::kOk;
}

// Read picture offsets from raw data.  CSGHeader::picOffset[1] is a flexible
// array — sizeof(CSGHeader) only covers one entry, so GetPicOffset(i) reads
// uninitialized memory for i > 0.  This function reads offsets directly from
// the raw buffer and resolves them to absolute offsets.
std::vector<uint32_t> ReadPicOffsets(const uint8_t* data, size_t len) {
    if (len < 6) return {};
    uint8_t picCountByte = data[5];
    int picCount = picCountByte & kPicCountMask;
    if (picCount == 0 || picCount > kCsgMaxPicCount) return {};
    bool useRelative = (picCountByte & kPicOffsetRelFlag) != 0;

    size_t offTableSize = static_cast<size_t>(picCount) * kCsgPicOffsetEntrySize;
    if (6 + offTableSize > len) return {};

    std::vector<uint32_t> offsets(picCount);
    if (useRelative) {
        offsets[0] = data[6] | (static_cast<uint32_t>(data[7]) << 8);
        for (int i = 1; i < picCount; ++i) {
            uint32_t rel = data[6 + i * 2]
                         | (static_cast<uint32_t>(data[6 + i * 2 + 1]) << 8);
            offsets[i] = offsets[i - 1] + rel;
            if (offsets[i] > kCsgMaxPicOffset) return {};
        }
    } else {
        for (int i = 0; i < picCount; ++i) {
            offsets[i] = data[6 + i * 2]
                       | (static_cast<uint32_t>(data[6 + i * 2 + 1]) << 8);
        }
    }
    return offsets;
}

CSG_ErrCode DecodePictureHeader(const uint8_t* data, size_t maxLen,
                                 CSGPicture& picture) {
    if (maxLen < 16)
        return CSG_ErrCode::kErrFileEof;

    std::memcpy(&picture, data, 16);

    // Verify magic
    if (picture.magic[0] != kCsgPictureMagic[0]
        || picture.magic[1] != kCsgPictureMagic[1])
        return CSG_ErrCode::kErrPicMagic;

    // Verify dimensions
    if (picture.width < 10 || picture.width > kCsgMaxWidth)
        return CSG_ErrCode::kErrWidthInvalid;
    if (picture.height < 10 || picture.height > kCsgMaxHeight)
        return CSG_ErrCode::kErrHeightInvalid;

    // Verify color mode
    if (!IsValidColorMode(picture.colorMode))
        return CSG_ErrCode::kErrCrmInvalid;

    // Verify CRN
    if (!IsValidCrn(picture.crn))
        return CSG_ErrCode::kErrCrnInvalid;

    // Verify CAS
    auto cas = picture.GetCompressAlgorithm();
    if (static_cast<uint8_t>(cas) > 4)
        return CSG_ErrCode::kErrCasInvalid;

    // Verify offsets
    if (picture.ppos == 0 || !IsAligned4(picture.ppos))
        return CSG_ErrCode::kErrPposAlign;
    if (picture.dpos == 0 || !IsAligned4(picture.dpos))
        return CSG_ErrCode::kErrDposAlign;

    // Verify size
    if (picture.size == 0 || picture.size > kCsgMaxSinglePic)
        return CSG_ErrCode::kErrSinglePicTooLarge;

    // Verify CRC
    if (picture.crc16 != 0) {
        uint16_t computed = Crc16(data + 4, picture.size - 4);
        if (computed != picture.crc16)
            return CSG_ErrCode::kErrPicCrc;
    }

    return CSG_ErrCode::kOk;
}


#ifndef __ARMCC_VERSION  // PC-only encoder: std::wstring, ostringstream, try/catch
// ============================================================================
// .csg file I/O
// ============================================================================

bool WriteCsgFile(const std::wstring& path, const CSGAtlas& atlas) {
    try {
        WriteBinaryFile(path, atlas.bytes);
        return true;
    } catch (...) {
        return false;
    }
}

CSG_ErrCode ReadCsgFile(const std::wstring& path, CSGAtlas& atlas) {
    // Read the file
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return CSG_ErrCode::kErrFileOpen;

    auto size = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()),
           static_cast<std::streamsize>(size));
    if (f.fail() && !f.eof())
        return CSG_ErrCode::kErrFileRead;

    f.close();

    // Decode header
    CSGHeader header;
    CSG_ErrCode err = DecodeAtlasHeader(data.data(), data.size(), header);
    if (err != CSG_ErrCode::kOk) return err;

    // Read offsets directly — CSGHeader::picOffset[1] can't hold >1 entry
    std::vector<uint32_t> offsets = ReadPicOffsets(data.data(), data.size());
    if (offsets.empty())
        return CSG_ErrCode::kErrOffsetTableBroken;

    atlas.bytes = std::move(data);
    atlas.pictures.clear();

    int picCount = static_cast<int>(offsets.size());
    for (int i = 0; i < picCount; ++i) {
        uint32_t offset = offsets[i];
        if (offset >= atlas.bytes.size())
            return CSG_ErrCode::kErrOffsetTableBroken;

        CSGPicture picHeader;
        err = DecodePictureHeader(atlas.bytes.data() + offset,
                                   atlas.bytes.size() - offset,
                                   picHeader);
        if (err != CSG_ErrCode::kOk) return err;

        CSGPictureEncodeData pic;
        pic.targetWidth  = picHeader.width;
        pic.targetHeight = picHeader.height;
        pic.colorMode    = static_cast<ColorMode>(picHeader.colorMode);
        pic.crn          = picHeader.crn;
        pic.algo         = picHeader.GetCompressAlgorithm();
        pic.cal          = picHeader.GetCompressLevel();
        pic.pictureCrc16 = picHeader.crc16;

        // Copy encoded bytes
        size_t picEnd = offset + picHeader.size;
        if (picEnd > atlas.bytes.size())
            return CSG_ErrCode::kErrPicSizeOverflow;

        pic.encodedPicture.assign(atlas.bytes.begin() + offset,
                                  atlas.bytes.begin() + picEnd);
        pic.encodedSize = picHeader.size;

        atlas.pictures.push_back(std::move(pic));
    }

    return CSG_ErrCode::kOk;
}

// ============================================================================
// C++ source export
// ============================================================================

std::string FormatByteArray(const uint8_t* data, size_t len,
                            const std::string& indent) {
    // Build a full template line to measure its length for trailing-comment
    // alignment on the final (possibly shorter) line.
    std::ostringstream tmpl;
    tmpl << indent;
    for (int b = 0; b < kCppExportBytesPerLine && static_cast<size_t>(b) < len; ++b) {
        tmpl << "0x00";
        if (b + 1 < kCppExportBytesPerLine && static_cast<size_t>(b + 1) < len)
            tmpl << ", ";
    }
    size_t fullLineLen = tmpl.str().size();

    std::ostringstream ss;
    std::ostringstream lineBuf;  // accumulates one line of hex bytes
    int bytesOnLine = 0;
    size_t firstIdxOnLine = 0;  // 0-based index of first byte on current line

    auto flushLine = [&](bool isLast) {
        std::string hexPart = lineBuf.str();
        ss << hexPart;
        if (isLast && bytesOnLine < kCppExportBytesPerLine) {
            // Pad to align trailing comment with full-length lines
            if (hexPart.size() < fullLineLen)
                ss << std::string(fullLineLen - hexPart.size(), ' ');
        }
        // Trailing comment: 1-based index of the last byte on this line
        size_t lastIdx = firstIdxOnLine + bytesOnLine;  // 1-based
        ss << "   // " << lastIdx << "\n";
        lineBuf.str("");
        lineBuf.clear();
        firstIdxOnLine += bytesOnLine;
        bytesOnLine = 0;
    };

    for (size_t i = 0; i < len; ++i) {
        if (bytesOnLine == 0)
            lineBuf << indent;
        lineBuf << "0x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]);
        ++bytesOnLine;
        if (i + 1 < len)
            lineBuf << ", ";
        if (bytesOnLine == kCppExportBytesPerLine)
            flushLine(/*isLast=*/false);
    }
    if (bytesOnLine > 0)
        flushLine(/*isLast=*/true);

    ss << std::dec;
    return ss.str();
}

static std::string GeneratePictureComment(const CSGPictureEncodeData& pic) {
    // Compression ratio: encoded size / source size * 100 (2 decimal places)
    double ratio = 0.0;
    if (pic.srcFileSize > 0)
        ratio = static_cast<double>(pic.encodedSize) * 100.0
                / static_cast<double>(pic.srcFileSize);

    // Helper: emit one comment line with colon-aligned label (right-justified)
    auto line = [](std::ostringstream& s, const char* label,
                   const std::string& value) {
        s << "   " << std::right << std::setw(14) << label << " : " << value << "\n";
    };
    auto lineInt = [&](std::ostringstream& s, const char* label, int value,
                       const char* suffix = "") {
        line(s, label, std::to_string(value) + suffix);
    };

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "// ============================================================\n"
       << "/*\n";
    line(ss, "Original image", pic.imageFileName);
    lineInt(ss, "width",  pic.srcWidth);
    lineInt(ss, "height", pic.srcHeight);
    lineInt(ss, "size",   pic.srcFileSize, " bytes");
    ss << "   ---------------------------------------------\n";
    line(ss, "CSG image",    pic.pictureName);
    line(ss, "version",
         std::to_string(static_cast<int>(kCsgVersionMajor)) + "."
         + std::to_string(static_cast<int>(kCsgVersionMinor)));
    lineInt(ss, "size",      pic.encodedSize, " bytes");
    {
        std::ostringstream rs;
        rs << std::fixed << std::setprecision(2) << ratio << " %";
        line(ss, "ratio", rs.str());
    }
    line(ss, "compress",     CompressModeName(pic.algo));
    lineInt(ss, "color number", pic.crn);
    line(ss, "color mode",   ColorModeName(pic.colorMode));
    ss << "*/\n";
    return ss.str();
}

static std::string GeneratePictureDataArray(
    const CSGPictureEncodeData& pic, const std::string& dataName) {

    std::ostringstream ss;
    ss << "static const unsigned char " << dataName << "[] = {\n"
       << FormatByteArray(pic.encodedPicture.data(),
                          pic.encodedPicture.size(), "    ")
       << "};\n\n";
    return ss.str();
}

static std::string GeneratePictureIndex(
    const CSGPictureEncodeData& pic, const std::string& dataName) {

    std::string picName = "pic" + pic.pictureName + "csg";
    std::ostringstream ss;
    ss << "// ------------------------------------------------------------\n"
       << "// " << pic.pictureName << " indexer\n"
       << "const TGUIPicture " << picName << " = {\n"
       << "    ID_CSG,             // Type\n"
       << "    " << static_cast<int>(kCsgCurVersion) << ",                // Version\n"
       << "    " << pic.encodedSize << ",  // Size\n"
       << "    " << dataName << "  // pData\n"
       << "};\n\n";
    return ss.str();
}

bool ExportCppSource(const std::wstring& cppPath,
                     const std::wstring& hPath,
                     const CSGAtlas& atlas,
                     const std::string& symbolName) {

    try {
        // ---- Generate .cpp ----
        std::ostringstream cpp;
        cpp << GenerateCppCaption(symbolName);

        for (size_t i = 0; i < atlas.pictures.size(); ++i) {
            const auto& pic = atlas.pictures[i];
            std::string dataName = "ac" + pic.pictureName;

            cpp << GeneratePictureComment(pic);
            cpp << GeneratePictureDataArray(pic, dataName);
            cpp << GeneratePictureIndex(pic, dataName);
        }

        WriteTextFile(cppPath, cpp.str());

        // ---- Generate .h ----
        std::vector<std::string> picNames;
        for (const auto& pic : atlas.pictures)
            picNames.push_back("pic" + pic.pictureName + "csg");

        std::string hdr = GenerateHeaderGuard(
            symbolName, symbolName, picNames);
        WriteTextFile(hPath, hdr);

        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Indices pack helper (for no-compression mode)
// ============================================================================

static std::vector<uint8_t> IndicesPack(const std::vector<int>& indices, int crn) {
    int bpi = BitsPerIndex(crn);
    if (bpi < 1) return {};

    BitPacker bp;
    for (int idx : indices)
        bp.PackBits(static_cast<uint8_t>(idx & 0xFF), bpi);
    return bp.GetBytes();
}

#endif  // __ARMCC_VERSION
