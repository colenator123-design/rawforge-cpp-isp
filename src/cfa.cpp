#include "rawforge/cfa.hpp"

namespace rawforge {

Color quad_rggb_color(const std::size_t x, const std::size_t y) noexcept {
    const auto block_x = (x % 4U) / 2U;
    const auto block_y = (y % 4U) / 2U;
    if (block_x == 0U && block_y == 0U) {
        return Color::Red;
    }
    if (block_x == 1U && block_y == 1U) {
        return Color::Blue;
    }
    return Color::Green;
}

Color bayer_rggb_color(const std::size_t x, const std::size_t y) noexcept {
    if (x % 2U == 0U && y % 2U == 0U) {
        return Color::Red;
    }
    if (x % 2U == 1U && y % 2U == 1U) {
        return Color::Blue;
    }
    return Color::Green;
}

}  // namespace rawforge

