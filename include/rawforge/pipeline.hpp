#pragma once

#include "rawforge/image.hpp"

namespace rawforge {

struct DenoiseParameters {
    int radius = 3;
    float spatial_sigma = 2.5F;
    float range_sigma = 0.12F;
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
