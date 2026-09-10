#pragma once

#include <cstdint>

#include "rawforge/image.hpp"

namespace rawforge {

[[nodiscard]] Image<float> create_test_scene(std::size_t width, std::size_t height);
[[nodiscard]] Image<float> mosaic_quad_rggb(const Image<float>& rgb);
[[nodiscard]] Image<float> mosaic_bayer_rggb(const Image<float>& rgb);
[[nodiscard]] Image<float> add_sensor_noise(
    const Image<float>& clean,
    float read_sigma = 0.035F,
    float shot_scale = 0.025F,
    std::uint32_t seed = 2026
);

}  // namespace rawforge

