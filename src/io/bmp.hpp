#pragma once

#include <cstdint>
#include <string>

namespace paint
{

// Writes an RGBA8888 pixel buffer (top-down rows) as a 24-bit bottom-up BMP.
// Returns true on success.
[[nodiscard]] bool writeBMPFile(const std::string &name, const uint8_t *rgba, int w, int h);

}
