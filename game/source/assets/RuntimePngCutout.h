#pragma once

// M96 Ground Cover cutout compatibility. Inspects PNG-declared transparency
// and decoded 8-bit pixels. Not a material system, not Content Browser
// metadata, and not a generalized texture classifier.

#include "assets/RuntimePng.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace assets
{
inline constexpr std::uint8_t kRuntimePngCutoutAlphaThreshold = 128;

namespace png_detail
{
struct BitReader
{
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t offset = 0;
    std::uint32_t bits = 0;
    int bitCount = 0;
};

inline bool ReaderFill(BitReader& reader, int needed)
{
    while (reader.bitCount < needed)
    {
        if (reader.offset >= reader.size)
        {
            return false;
        }
        reader.bits |= static_cast<std::uint32_t>(reader.data[reader.offset])
            << reader.bitCount;
        ++reader.offset;
        reader.bitCount += 8;
    }
    return true;
}

inline bool ReadBits(BitReader& reader, int count, std::uint32_t& value)
{
    if (count < 0 || count > 24 || !ReaderFill(reader, count))
    {
        return false;
    }
    if (count == 0)
    {
        value = 0;
        return true;
    }
    value = reader.bits & ((1u << count) - 1u);
    reader.bits >>= count;
    reader.bitCount -= count;
    return true;
}

inline bool DropToByteBoundary(BitReader& reader)
{
    const int drop = reader.bitCount & 7;
    if (drop == 0)
    {
        return true;
    }
    std::uint32_t ignored = 0;
    return ReadBits(reader, drop, ignored);
}

struct Huffman
{
    int counts[16]{};
    int firstCode[16]{};
    int firstSymbol[16]{};
    std::uint16_t symbols[288]{};
};

inline bool BuildHuffman(Huffman& table, const int* lengths, int count)
{
    std::memset(&table, 0, sizeof(table));
    if (lengths == nullptr || count <= 0 || count > 288)
    {
        return false;
    }
    for (int index = 0; index < count; ++index)
    {
        const int length = lengths[index];
        if (length < 0 || length > 15)
        {
            return false;
        }
        ++table.counts[length];
    }
    table.counts[0] = 0;
    int code = 0;
    int symbol = 0;
    for (int length = 1; length <= 15; ++length)
    {
        table.firstCode[length] = code;
        table.firstSymbol[length] = symbol;
        code = (code + table.counts[length]) << 1;
        symbol += table.counts[length];
    }
    int next[16]{};
    for (int length = 1; length <= 15; ++length)
    {
        next[length] = table.firstSymbol[length];
    }
    for (int index = 0; index < count; ++index)
    {
        const int length = lengths[index];
        if (length == 0)
        {
            continue;
        }
        table.symbols[next[length]++] = static_cast<std::uint16_t>(index);
    }
    return true;
}

inline bool DecodeHuffman(BitReader& reader, const Huffman& table, int& symbol)
{
    int code = 0;
    for (int length = 1; length <= 15; ++length)
    {
        std::uint32_t bit = 0;
        if (!ReadBits(reader, 1, bit))
        {
            return false;
        }
        code = (code << 1) | static_cast<int>(bit);
        const int index = code - table.firstCode[length];
        if (index >= 0 && index < table.counts[length])
        {
            symbol = table.symbols[table.firstSymbol[length] + index];
            return true;
        }
    }
    return false;
}

inline bool InflateZlib(
    const std::uint8_t* src, std::size_t srcSize, std::vector<std::uint8_t>& dest)
{
    dest.clear();
    if (src == nullptr || srcSize < 6)
    {
        return false;
    }
    const int cmf = src[0];
    const int flg = src[1];
    if ((cmf & 0x0F) != 8 || ((cmf << 8) + flg) % 31 != 0 || (flg & 0x20) != 0)
    {
        return false;
    }

    BitReader reader{};
    reader.data = src + 2;
    reader.size = srcSize - 6;
    static constexpr int kLengthBase[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83,
        99, 115, 131, 163, 195, 227, 258};
    static constexpr int kLengthExtra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
        0};
    static constexpr int kDistBase[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025,
        1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static constexpr int kDistExtra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
        12, 13, 13};

    bool done = false;
    while (!done)
    {
        std::uint32_t bfinal = 0;
        std::uint32_t btype = 0;
        if (!ReadBits(reader, 1, bfinal) || !ReadBits(reader, 2, btype))
        {
            return false;
        }
        done = bfinal != 0;
        if (btype == 3)
        {
            return false;
        }
        if (btype == 0)
        {
            if (!DropToByteBoundary(reader))
            {
                return false;
            }
            std::uint32_t len = 0;
            std::uint32_t nlen = 0;
            if (!ReadBits(reader, 16, len) || !ReadBits(reader, 16, nlen))
            {
                return false;
            }
            if ((len ^ 0xFFFFu) != nlen)
            {
                return false;
            }
            for (std::uint32_t index = 0; index < len; ++index)
            {
                std::uint32_t byte = 0;
                if (!ReadBits(reader, 8, byte))
                {
                    return false;
                }
                dest.push_back(static_cast<std::uint8_t>(byte));
            }
            continue;
        }

        int lengths[320]{};
        int litCount = 288;
        int distCount = 32;
        if (btype == 1)
        {
            for (int index = 0; index <= 143; ++index)
            {
                lengths[index] = 8;
            }
            for (int index = 144; index <= 255; ++index)
            {
                lengths[index] = 9;
            }
            for (int index = 256; index <= 279; ++index)
            {
                lengths[index] = 7;
            }
            for (int index = 280; index <= 287; ++index)
            {
                lengths[index] = 8;
            }
            for (int index = 0; index < 32; ++index)
            {
                lengths[288 + index] = 5;
            }
        }
        else
        {
            std::uint32_t hlit = 0;
            std::uint32_t hdist = 0;
            std::uint32_t hclen = 0;
            if (!ReadBits(reader, 5, hlit) || !ReadBits(reader, 5, hdist)
                || !ReadBits(reader, 4, hclen))
            {
                return false;
            }
            litCount = static_cast<int>(hlit) + 257;
            distCount = static_cast<int>(hdist) + 1;
            const int clenCount = static_cast<int>(hclen) + 4;
            if (litCount > 286 || distCount > 32 || clenCount > 19)
            {
                return false;
            }
            static constexpr int kClenOrder[19] = {
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            int clen[19]{};
            for (int index = 0; index < clenCount; ++index)
            {
                std::uint32_t bits = 0;
                if (!ReadBits(reader, 3, bits))
                {
                    return false;
                }
                clen[kClenOrder[index]] = static_cast<int>(bits);
            }
            Huffman clenTable{};
            if (!BuildHuffman(clenTable, clen, 19))
            {
                return false;
            }
            const int total = litCount + distCount;
            int filled = 0;
            while (filled < total)
            {
                int symbol = 0;
                if (!DecodeHuffman(reader, clenTable, symbol))
                {
                    return false;
                }
                int repeat = 1;
                int value = 0;
                if (symbol <= 15)
                {
                    value = symbol;
                }
                else if (symbol == 16)
                {
                    if (filled == 0)
                    {
                        return false;
                    }
                    value = lengths[filled - 1];
                    std::uint32_t extra = 0;
                    if (!ReadBits(reader, 2, extra))
                    {
                        return false;
                    }
                    repeat = static_cast<int>(extra) + 3;
                }
                else if (symbol == 17)
                {
                    value = 0;
                    std::uint32_t extra = 0;
                    if (!ReadBits(reader, 3, extra))
                    {
                        return false;
                    }
                    repeat = static_cast<int>(extra) + 3;
                }
                else if (symbol == 18)
                {
                    value = 0;
                    std::uint32_t extra = 0;
                    if (!ReadBits(reader, 7, extra))
                    {
                        return false;
                    }
                    repeat = static_cast<int>(extra) + 11;
                }
                else
                {
                    return false;
                }
                if (filled + repeat > total)
                {
                    return false;
                }
                for (int index = 0; index < repeat; ++index)
                {
                    lengths[filled++] = value;
                }
            }
        }

        Huffman lit{};
        Huffman dist{};
        if (!BuildHuffman(lit, lengths, litCount)
            || !BuildHuffman(dist, lengths + litCount, distCount))
        {
            return false;
        }

        for (;;)
        {
            int symbol = 0;
            if (!DecodeHuffman(reader, lit, symbol))
            {
                return false;
            }
            if (symbol < 256)
            {
                dest.push_back(static_cast<std::uint8_t>(symbol));
                continue;
            }
            if (symbol == 256)
            {
                break;
            }
            if (symbol > 285)
            {
                return false;
            }
            const int lengthIndex = symbol - 257;
            std::uint32_t extraLen = 0;
            if (!ReadBits(reader, kLengthExtra[lengthIndex], extraLen))
            {
                return false;
            }
            const int copyLength = kLengthBase[lengthIndex] + static_cast<int>(extraLen);
            int distSymbol = 0;
            if (!DecodeHuffman(reader, dist, distSymbol) || distSymbol < 0 || distSymbol > 29)
            {
                return false;
            }
            std::uint32_t extraDist = 0;
            if (!ReadBits(reader, kDistExtra[distSymbol], extraDist))
            {
                return false;
            }
            const int distance = kDistBase[distSymbol] + static_cast<int>(extraDist);
            if (distance <= 0 || static_cast<std::size_t>(distance) > dest.size())
            {
                return false;
            }
            for (int index = 0; index < copyLength; ++index)
            {
                dest.push_back(dest[dest.size() - static_cast<std::size_t>(distance)]);
            }
        }
    }
    return true;
}

inline std::uint8_t Paeth(std::uint8_t a, std::uint8_t b, std::uint8_t c)
{
    const int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);
    const int pa = p >= static_cast<int>(a) ? p - static_cast<int>(a) : static_cast<int>(a) - p;
    const int pb = p >= static_cast<int>(b) ? p - static_cast<int>(b) : static_cast<int>(b) - p;
    const int pc = p >= static_cast<int>(c) ? p - static_cast<int>(c) : static_cast<int>(c) - p;
    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    if (pb <= pc)
    {
        return b;
    }
    return c;
}

inline bool UnfilterScanlines(
    std::vector<std::uint8_t>& filtered, int width, int height, int bytesPerPixel)
{
    if (width <= 0 || height <= 0 || bytesPerPixel <= 0)
    {
        return false;
    }
    const std::size_t stride = static_cast<std::size_t>(width) * static_cast<std::size_t>(bytesPerPixel);
    const std::size_t expected = (stride + 1) * static_cast<std::size_t>(height);
    if (filtered.size() < expected)
    {
        return false;
    }
    std::vector<std::uint8_t> recon(stride * static_cast<std::size_t>(height), 0);
    for (int row = 0; row < height; ++row)
    {
        const std::size_t srcRow = static_cast<std::size_t>(row) * (stride + 1);
        const int filter = filtered[srcRow];
        const std::uint8_t* src = filtered.data() + srcRow + 1;
        std::uint8_t* dst = recon.data() + static_cast<std::size_t>(row) * stride;
        const std::uint8_t* prior =
            row == 0 ? nullptr : recon.data() + static_cast<std::size_t>(row - 1) * stride;
        if (filter > 4)
        {
            return false;
        }
        for (std::size_t index = 0; index < stride; ++index)
        {
            const std::uint8_t raw = src[index];
            const std::uint8_t left =
                index >= static_cast<std::size_t>(bytesPerPixel) ? dst[index - static_cast<std::size_t>(bytesPerPixel)]
                                                                : 0;
            const std::uint8_t up = prior != nullptr ? prior[index] : 0;
            const std::uint8_t upLeft =
                prior != nullptr && index >= static_cast<std::size_t>(bytesPerPixel)
                    ? prior[index - static_cast<std::size_t>(bytesPerPixel)]
                    : 0;
            std::uint8_t value = raw;
            if (filter == 1)
            {
                value = static_cast<std::uint8_t>(raw + left);
            }
            else if (filter == 2)
            {
                value = static_cast<std::uint8_t>(raw + up);
            }
            else if (filter == 3)
            {
                value = static_cast<std::uint8_t>(
                    raw + static_cast<std::uint8_t>((static_cast<int>(left) + static_cast<int>(up)) / 2));
            }
            else if (filter == 4)
            {
                value = static_cast<std::uint8_t>(raw + Paeth(left, up, upLeft));
            }
            dst[index] = value;
        }
    }
    filtered.swap(recon);
    return true;
}

inline std::uint32_t ReadBe32(const std::uint8_t* data)
{
    return (static_cast<std::uint32_t>(data[0]) << 24) | (static_cast<std::uint32_t>(data[1]) << 16)
        | (static_cast<std::uint32_t>(data[2]) << 8) | static_cast<std::uint32_t>(data[3]);
}

inline bool DecodePng8Bit(
    const std::uint8_t* data,
    std::size_t size,
    int& width,
    int& height,
    int& colorType,
    bool& hasTrns,
    std::uint16_t trnsRgb[3],
    std::vector<std::uint8_t>& pixels)
{
    width = 0;
    height = 0;
    colorType = 0;
    hasTrns = false;
    trnsRgb[0] = 0;
    trnsRgb[1] = 0;
    trnsRgb[2] = 0;
    pixels.clear();
    int bitDepth = 0;
    if (!TryReadRuntimePngIhdr(data, size, width, height, bitDepth, colorType, nullptr))
    {
        return false;
    }
    if (bitDepth != 8 || data[26] != 0 || data[27] != 0 || data[28] != 0)
    {
        return false;
    }

    std::vector<std::uint8_t> idat;
    std::size_t offset = 8;
    bool seenIend = false;
    while (offset + 12 <= size)
    {
        const std::uint32_t length = ReadBe32(data + offset);
        if (offset + 12 + length > size)
        {
            return false;
        }
        const char type0 = static_cast<char>(data[offset + 4]);
        const char type1 = static_cast<char>(data[offset + 5]);
        const char type2 = static_cast<char>(data[offset + 6]);
        const char type3 = static_cast<char>(data[offset + 7]);
        const std::uint8_t* chunk = data + offset + 8;
        if (type0 == 'I' && type1 == 'D' && type2 == 'A' && type3 == 'T')
        {
            idat.insert(idat.end(), chunk, chunk + length);
        }
        else if (type0 == 't' && type1 == 'R' && type2 == 'N' && type3 == 'S')
        {
            if (colorType == 2 && length >= 6)
            {
                hasTrns = true;
                trnsRgb[0] = static_cast<std::uint16_t>((chunk[0] << 8) | chunk[1]);
                trnsRgb[1] = static_cast<std::uint16_t>((chunk[2] << 8) | chunk[3]);
                trnsRgb[2] = static_cast<std::uint16_t>((chunk[4] << 8) | chunk[5]);
            }
        }
        else if (type0 == 'I' && type1 == 'E' && type2 == 'N' && type3 == 'D')
        {
            seenIend = true;
            break;
        }
        offset += 12 + length;
    }
    if (!seenIend || idat.empty())
    {
        return false;
    }

    int bytesPerPixel = 0;
    if (colorType == 2)
    {
        bytesPerPixel = 3;
    }
    else if (colorType == 4)
    {
        bytesPerPixel = 2;
    }
    else if (colorType == 6)
    {
        bytesPerPixel = 4;
    }
    else
    {
        return false;
    }

    if (!InflateZlib(idat.data(), idat.size(), pixels))
    {
        return false;
    }
    return UnfilterScanlines(pixels, width, height, bytesPerPixel);
}
}

inline bool RuntimePngHasUsefulCutoutAlpha(const std::uint8_t* data, std::size_t size)
{
    int headerWidth = 0;
    int headerHeight = 0;
    int bitDepth = 0;
    int headerColorType = 0;
    if (!TryReadRuntimePngIhdr(
            data, size, headerWidth, headerHeight, bitDepth, headerColorType, nullptr)
        || bitDepth != 8)
    {
        return false;
    }
    if (headerColorType != 2 && headerColorType != 4 && headerColorType != 6)
    {
        return false;
    }
    if (headerColorType == 2)
    {
        bool declaresRgbTransparency = false;
        std::size_t offset = 8;
        while (offset + 12 <= size)
        {
            const std::uint32_t length = png_detail::ReadBe32(data + offset);
            if (offset + 12 + static_cast<std::size_t>(length) > size)
            {
                break;
            }
            if (data[offset + 4] == 't' && data[offset + 5] == 'R' && data[offset + 6] == 'N'
                && data[offset + 7] == 'S' && length >= 6)
            {
                declaresRgbTransparency = true;
                break;
            }
            if (data[offset + 4] == 'I' && data[offset + 5] == 'E' && data[offset + 6] == 'N'
                && data[offset + 7] == 'D')
            {
                break;
            }
            offset += 12 + static_cast<std::size_t>(length);
        }
        if (!declaresRgbTransparency)
        {
            return false;
        }
    }

    int width = 0;
    int height = 0;
    int colorType = 0;
    bool hasTrns = false;
    std::uint16_t trnsRgb[3]{};
    std::vector<std::uint8_t> pixels;
    if (!png_detail::DecodePng8Bit(data, size, width, height, colorType, hasTrns, trnsRgb, pixels))
    {
        return false;
    }
    if (colorType == 2 && !hasTrns)
    {
        return false;
    }
    const int pixelCount = width * height;
    if (colorType == 6)
    {
        for (int index = 0; index < pixelCount; ++index)
        {
            if (pixels[static_cast<std::size_t>(index) * 4 + 3] < kRuntimePngCutoutAlphaThreshold)
            {
                return true;
            }
        }
        return false;
    }
    if (colorType == 4)
    {
        for (int index = 0; index < pixelCount; ++index)
        {
            if (pixels[static_cast<std::size_t>(index) * 2 + 1] < kRuntimePngCutoutAlphaThreshold)
            {
                return true;
            }
        }
        return false;
    }
    if (colorType == 2 && hasTrns)
    {
        const std::uint8_t tr = static_cast<std::uint8_t>(trnsRgb[0] > 255 ? 255 : trnsRgb[0]);
        const std::uint8_t tg = static_cast<std::uint8_t>(trnsRgb[1] > 255 ? 255 : trnsRgb[1]);
        const std::uint8_t tb = static_cast<std::uint8_t>(trnsRgb[2] > 255 ? 255 : trnsRgb[2]);
        for (int index = 0; index < pixelCount; ++index)
        {
            const std::size_t offset = static_cast<std::size_t>(index) * 3;
            if (pixels[offset] == tr && pixels[offset + 1] == tg && pixels[offset + 2] == tb)
            {
                return true;
            }
        }
    }
    return false;
}

inline bool RuntimePngHasUsefulCutoutAlpha(const std::vector<std::uint8_t>& bytes)
{
    return RuntimePngHasUsefulCutoutAlpha(bytes.data(), bytes.size());
}

inline bool RuntimePngFileHasUsefulCutoutAlpha(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return false;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size <= 0)
    {
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!stream)
    {
        return false;
    }
    return RuntimePngHasUsefulCutoutAlpha(bytes.data(), bytes.size());
}
}
