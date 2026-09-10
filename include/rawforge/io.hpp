#pragma once

#include <filesystem>

#include "rawforge/image.hpp"

namespace rawforge {

void write_pgm(const std::filesystem::path& path, const Image<float>& image);
void write_ppm(const std::filesystem::path& path, const Image<float>& rgb);

}  // namespace rawforge

