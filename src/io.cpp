#include "rawforge/io.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace rawforge {
namespace {

std::uint8_t encode(const float linear) {
    const float srgb = linear <= 0.0031308F ? 12.92F * linear
                                            : 1.055F * std::pow(linear, 1.0F / 2.4F) - 0.055F;
    return static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround(srgb * 255.0F)), 0, 255)
    );
}

}  // namespace

void write_pgm(const std::filesystem::path& path, const Image<float>& image) {
    if (image.channels() != 1) {
        throw std::invalid_argument("PGM output requires one channel");
    }
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream << "P5\n" << image.width() << ' ' << image.height() << "\n255\n";
    for (const float value : image.data()) {
        stream.put(static_cast<char>(encode(std::clamp(value, 0.0F, 1.0F))));
    }
}

void write_ppm(const std::filesystem::path& path, const Image<float>& rgb) {
    if (rgb.channels() != 3) {
        throw std::invalid_argument("PPM output requires three channels");
    }
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream << "P6\n" << rgb.width() << ' ' << rgb.height() << "\n255\n";
    for (const float value : rgb.data()) {
        stream.put(static_cast<char>(encode(std::clamp(value, 0.0F, 1.0F))));
    }
}

}  // namespace rawforge
