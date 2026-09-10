#pragma once

#include <cstddef>

namespace rawforge {

enum class Color : std::size_t { Red = 0, Green = 1, Blue = 2 };

[[nodiscard]] Color quad_rggb_color(std::size_t x, std::size_t y) noexcept;
[[nodiscard]] Color bayer_rggb_color(std::size_t x, std::size_t y) noexcept;

}  // namespace rawforge

