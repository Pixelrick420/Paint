#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "bmp.hpp"

namespace paint
{

namespace
{

void putU16(uint8_t *dst, uint16_t v) { std::memcpy(dst, &v, sizeof(v)); }

void putU32(uint8_t *dst, uint32_t v) { std::memcpy(dst, &v, sizeof(v)); }

constexpr uint32_t BMP_HEADER_SIZE = 54;

} // namespace

bool writeBMPFile(const std::string &name, const uint8_t *rgba, int w, int h)
{
    if (w <= 0 || h <= 0 || rgba == nullptr)
        return false;

    std::ofstream out(name, std::ios::binary);
    if (!out)
    {
        std::cerr << "Save failed: cannot open " << name << std::endl;
        return false;
    }

    int rowSize = (w * 3 + 3) & ~3;
    uint32_t imageSize = static_cast<uint32_t>(rowSize * h);
    std::array<uint8_t, BMP_HEADER_SIZE> header{};
    header[0] = 'B';
    header[1] = 'M';
    putU32(header.data() + 2, imageSize + BMP_HEADER_SIZE);      // file size
    putU32(header.data() + 10, BMP_HEADER_SIZE);                 // pixel data offset
    putU32(header.data() + 14, 40);                              // BITMAPINFOHEADER size
    putU32(header.data() + 18, static_cast<uint32_t>(w));        // width
    putU32(header.data() + 22, static_cast<uint32_t>(h));        // height
    putU16(header.data() + 26, 1);                               // color planes
    putU16(header.data() + 28, 24);                              // bits per pixel
    putU32(header.data() + 34, imageSize);                       // raw image size
    out.write(reinterpret_cast<const char *>(header.data()),
              static_cast<std::streamsize>(header.size()));

    std::vector<uint8_t> row(rowSize);
    for (int y = h - 1; y >= 0; --y)
    {
        const uint8_t *src = rgba + static_cast<size_t>(y) * w * 4;
        for (int x = 0; x < w; ++x)
        {
            row[x * 3 + 0] = src[x * 4 + 2]; // B
            row[x * 3 + 1] = src[x * 4 + 1]; // G
            row[x * 3 + 2] = src[x * 4 + 0]; // R
        }
        out.write(reinterpret_cast<const char *>(row.data()),
                  static_cast<std::streamsize>(row.size()));
    }
    return true;
}

}
