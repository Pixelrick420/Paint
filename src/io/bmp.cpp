#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "bmp.hpp"

namespace paint
{

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
    int imageSize = rowSize * h;
    uint8_t header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    uint32_t fsize = 54 + imageSize;
    std::memcpy(header + 2, &fsize, 4);
    uint32_t dataOff = 54;
    std::memcpy(header + 10, &dataOff, 4);
    uint32_t ih = 40;
    std::memcpy(header + 14, &ih, 4);
    int32_t iw = w, ihd = h;
    std::memcpy(header + 18, &iw, 4);
    std::memcpy(header + 22, &ihd, 4);
    uint16_t planes = 1;
    std::memcpy(header + 26, &planes, 2);
    uint16_t bpp = 24;
    std::memcpy(header + 28, &bpp, 2);
    uint32_t comp = 0;
    std::memcpy(header + 30, &comp, 4);
    std::memcpy(header + 34, &imageSize, 4);
    out.write((char *)header, 54);

    std::vector<uint8_t> row(rowSize);
    for (int y = h - 1; y >= 0; --y)
    {
        const uint8_t *src = rgba + (size_t)y * w * 4;
        for (int x = 0; x < w; ++x)
        {
            row[x * 3 + 0] = src[x * 4 + 2]; // B
            row[x * 3 + 1] = src[x * 4 + 1]; // G
            row[x * 3 + 2] = src[x * 4 + 0]; // R
        }
        out.write((char *)row.data(), rowSize);
    }
    out.close();
    return true;
}

}
