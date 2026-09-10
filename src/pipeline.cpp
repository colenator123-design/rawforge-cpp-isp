#include "rawforge/pipeline.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include "rawforge/cfa.hpp"

namespace rawforge {
namespace {

void require_raw(const Image<float>& image) {
    if (image.channels() != 1) {
        throw std::invalid_argument("RAW image must contain one channel");
    }
}

float interpolate_quad_color(
    const Image<float>& raw,
    const int center_x,
    const int center_y,
    const Color wanted
) {
    if (quad_rggb_color(center_x, center_y) == wanted) {
        return raw(center_x, center_y);
    }
    constexpr int radius = 3;
    float weighted_sum = 0.0F;
    float weight_sum = 0.0F;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int x = clamp_coordinate(center_x + dx, static_cast<int>(raw.width()));
            const int y = clamp_coordinate(center_y + dy, static_cast<int>(raw.height()));
            if (quad_rggb_color(x, y) != wanted) {
                continue;
            }
            const float distance_squared = static_cast<float>(dx * dx + dy * dy);
            const float weight = 1.0F / (1.0F + distance_squared);
            weighted_sum += weight * raw(x, y);
            weight_sum += weight;
        }
    }
    return weighted_sum / std::max(weight_sum, 1.0e-8F);
}

float interpolate_bayer_color(
    const Image<float>& raw,
    const int center_x,
    const int center_y,
    const Color wanted
) {
    if (bayer_rggb_color(center_x, center_y) == wanted) {
        return raw(center_x, center_y);
    }
    float weighted_sum = 0.0F;
    float weight_sum = 0.0F;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int x = clamp_coordinate(center_x + dx, static_cast<int>(raw.width()));
            const int y = clamp_coordinate(center_y + dy, static_cast<int>(raw.height()));
            if (bayer_rggb_color(x, y) != wanted) {
                continue;
            }
            const float distance_squared = static_cast<float>(dx * dx + dy * dy);
            const float weight = 1.0F / (1.0F + distance_squared);
            weighted_sum += weight * raw(x, y);
            weight_sum += weight;
        }
    }
    return weighted_sum / std::max(weight_sum, 1.0e-8F);
}

}  // namespace

Image<float> same_color_bilateral(
    const Image<float>& quad_raw,
    const DenoiseParameters& parameters
) {
    require_raw(quad_raw);
    if (parameters.radius < 1 || parameters.spatial_sigma <= 0.0F ||
        parameters.range_sigma <= 0.0F) {
        throw std::invalid_argument("Invalid denoise parameters");
    }
    Image<float> filtered(quad_raw.width(), quad_raw.height());
    const float spatial_denominator = 2.0F * parameters.spatial_sigma * parameters.spatial_sigma;
    const float range_denominator = 2.0F * parameters.range_sigma * parameters.range_sigma;
    for (int y = 0; y < static_cast<int>(quad_raw.height()); ++y) {
        for (int x = 0; x < static_cast<int>(quad_raw.width()); ++x) {
            const Color center_color = quad_rggb_color(x, y);
            const float center = quad_raw(x, y);
            float weighted_sum = 0.0F;
            float weight_sum = 0.0F;
            for (int dy = -parameters.radius; dy <= parameters.radius; ++dy) {
                for (int dx = -parameters.radius; dx <= parameters.radius; ++dx) {
                    const int neighbor_x =
                        clamp_coordinate(x + dx, static_cast<int>(quad_raw.width()));
                    const int neighbor_y =
                        clamp_coordinate(y + dy, static_cast<int>(quad_raw.height()));
                    if (quad_rggb_color(neighbor_x, neighbor_y) != center_color) {
                        continue;
                    }
                    const float neighbor = quad_raw(neighbor_x, neighbor_y);
                    const float spatial = std::exp(-static_cast<float>(dx * dx + dy * dy) /
                                                   spatial_denominator);
                    const float difference = neighbor - center;
                    const float range = std::exp(-(difference * difference) / range_denominator);
                    const float weight = spatial * range;
                    weighted_sum += weight * neighbor;
                    weight_sum += weight;
                }
            }
            filtered(x, y) = weighted_sum / std::max(weight_sum, 1.0e-8F);
        }
    }
    return filtered;
}

Image<float> remosaic_quad_to_bayer(const Image<float>& quad_raw) {
    require_raw(quad_raw);
    Image<float> bayer(quad_raw.width(), quad_raw.height());
    for (int y = 0; y < static_cast<int>(quad_raw.height()); ++y) {
        for (int x = 0; x < static_cast<int>(quad_raw.width()); ++x) {
            bayer(x, y) = interpolate_quad_color(quad_raw, x, y, bayer_rggb_color(x, y));
        }
    }
    return bayer;
}

Image<float> joint_remosaic_denoise(
    const Image<float>& quad_raw,
    const DenoiseParameters& parameters
) {
    return remosaic_quad_to_bayer(same_color_bilateral(quad_raw, parameters));
}

Image<float> demosaic_bayer_bilinear(const Image<float>& bayer_raw) {
    require_raw(bayer_raw);
    Image<float> rgb(bayer_raw.width(), bayer_raw.height(), 3);
    constexpr std::array<Color, 3> colors = {Color::Red, Color::Green, Color::Blue};
    for (int y = 0; y < static_cast<int>(bayer_raw.height()); ++y) {
        for (int x = 0; x < static_cast<int>(bayer_raw.width()); ++x) {
            for (std::size_t channel = 0; channel < colors.size(); ++channel) {
                rgb(x, y, channel) = interpolate_bayer_color(bayer_raw, x, y, colors[channel]);
            }
        }
    }
    return rgb;
}

}  // namespace rawforge

