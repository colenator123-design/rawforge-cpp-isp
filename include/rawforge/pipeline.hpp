#pragma once

#include "rawforge/image.hpp"

namespace rawforge {

struct DenoiseParameters {
    int radius = 4;
    float spatial_sigma = 3.5F;
    float read_noise_sigma = 0.035F;
    float shot_noise_scale = 0.025F;
    float range_multiplier = 1.5F;
    unsigned int threads = 0;
};

[[nodiscard]] Image<float> same_color_bilateral(
    const Image<float>& quad_raw,
    const DenoiseParameters& parameters = {}
);

[[nodiscard]] Image<float> remosaic_quad_to_bayer(const Image<float>& quad_raw);

[[nodiscard]] Image<float> joint_remosaic_denoise(
    const Image<float>& quad_raw,
    const DenoiseParameters& parameters = {}
);

[[nodiscard]] Image<float> demosaic_bayer_bilinear(const Image<float>& bayer_raw);

}  // namespace rawforge
