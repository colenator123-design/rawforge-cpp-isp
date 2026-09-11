#include "rawforge/pipeline.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <vector>

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
        parameters.read_noise_sigma <= 0.0F || parameters.shot_noise_scale < 0.0F ||
        parameters.range_multiplier <= 0.0F) {
        throw std::invalid_argument("Invalid denoise parameters");
    }
    Image<float> filtered(quad_raw.width(), quad_raw.height());
    const float spatial_denominator = 2.0F * parameters.spatial_sigma * parameters.spatial_sigma;
    const int window = 2 * parameters.radius + 1;
    std::vector<float> spatial_weights(static_cast<std::size_t>(window * window));
    for (int dy = -parameters.radius; dy <= parameters.radius; ++dy) {
        for (int dx = -parameters.radius; dx <= parameters.radius; ++dx) {
            const auto index = static_cast<std::size_t>(
                (dy + parameters.radius) * window + dx + parameters.radius
            );
            spatial_weights[index] =
                std::exp(-static_cast<float>(dx * dx + dy * dy) / spatial_denominator);
        }
    }

    const auto process_rows = [&](const int first_row, const int last_row) {
        for (int y = first_row; y < last_row; ++y) {
            for (int x = 0; x < static_cast<int>(quad_raw.width()); ++x) {
                const Color center_color = quad_rggb_color(x, y);
                const float center = quad_raw(x, y);
                const float noise_sigma = parameters.read_noise_sigma +
                                          parameters.shot_noise_scale *
                                              std::sqrt(std::max(center, 0.0F));
                const float range_sigma = parameters.range_multiplier * noise_sigma;
                const float range_denominator = 2.0F * range_sigma * range_sigma;
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
                        const auto spatial_index = static_cast<std::size_t>(
                            (dy + parameters.radius) * window + dx + parameters.radius
                        );
                        const float spatial = spatial_weights[spatial_index];
                        const float difference = neighbor - center;
                        const float range =
                            std::exp(-(difference * difference) / range_denominator);
                        const float weight = spatial * range;
                        weighted_sum += weight * neighbor;
                        weight_sum += weight;
                    }
                }
                filtered(x, y) = weighted_sum / std::max(weight_sum, 1.0e-8F);
            }
        }
    };

    const unsigned int available_threads = parameters.threads == 0
                                               ? std::thread::hardware_concurrency()
                                               : parameters.threads;
    const unsigned int thread_count = std::clamp(
        available_threads == 0 ? 1U : available_threads,
        1U,
        static_cast<unsigned int>(quad_raw.height())
    );
    if (thread_count == 1U) {
        process_rows(0, static_cast<int>(quad_raw.height()));
        return filtered;
    }
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    const int rows_per_thread =
        (static_cast<int>(quad_raw.height()) + static_cast<int>(thread_count) - 1) /
        static_cast<int>(thread_count);
    for (unsigned int thread_id = 0; thread_id < thread_count; ++thread_id) {
        const int first_row = static_cast<int>(thread_id) * rows_per_thread;
        const int last_row = std::min(
            first_row + rows_per_thread,
            static_cast<int>(quad_raw.height())
        );
        if (first_row < last_row) {
            workers.emplace_back(process_rows, first_row, last_row);
        }
    }
    for (auto& worker : workers) {
        worker.join();
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
