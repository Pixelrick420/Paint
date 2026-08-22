#pragma once

#include <cstdint>
#include <string>

namespace paint
{

// Writes top-down RGBA8888 pixels as a 24-bit bottom-up BMP.
[[nodiscard]] bool writeBMPFile(const std::string &name, const uint8_t *rgba, int w, int h);

}
