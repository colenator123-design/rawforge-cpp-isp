#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "rawforge/metrics.hpp"
#include "rawforge/pipeline.hpp"
#include "rawforge/synthetic.hpp"

struct NoiseProfile {
    float read_sigma;
    float shot_scale;
};

int main() {
    const std::vector<NoiseProfile> profiles = {
        {0.015F, 0.010F}, {0.035F, 0.025F}, {0.060F, 0.040F}
    };
    double best_average = -std::numeric_limits<double>::infinity();
    rawforge::DenoiseParameters best_parameters;
    double best_worst = 0.0;

    for (const int radius : {2, 3, 4}) {
        for (const float spatial_sigma : {1.5F, 2.5F, 3.5F}) {
            for (const float multiplier : {1.5F, 2.0F, 2.5F, 3.0F}) {
                double gain_sum = 0.0;
                double worst_gain = std::numeric_limits<double>::infinity();
                int cases = 0;
                for (std::uint32_t variant = 0; variant < 6; ++variant) {
                    const auto rgb = rawforge::create_test_scene(160, 120, variant);
                    const auto target = rawforge::mosaic_bayer_rggb(rgb);
                    const auto clean_quad = rawforge::mosaic_quad_rggb(rgb);
                    for (std::size_t profile_id = 0; profile_id < profiles.size(); ++profile_id) {
                        const auto profile = profiles[profile_id];
                        const auto noisy = rawforge::add_sensor_noise(
                            clean_quad,
                            profile.read_sigma,
                            profile.shot_scale,
                            2026U + variant * 17U + static_cast<std::uint32_t>(profile_id)
                        );
                        const auto baseline = rawforge::remosaic_quad_to_bayer(noisy);
                        const rawforge::DenoiseParameters parameters{
                            radius,
                            spatial_sigma,
                            profile.read_sigma,
                            profile.shot_scale,
                            multiplier,
                        };
                        const auto restored = rawforge::joint_remosaic_denoise(noisy, parameters);
                        const double gain =
                            rawforge::psnr(target, restored) - rawforge::psnr(target, baseline);
                        gain_sum += gain;
                        worst_gain = std::min(worst_gain, gain);
                        ++cases;
                    }
                }
                const double average_gain = gain_sum / static_cast<double>(cases);
                if (average_gain > best_average) {
                    best_average = average_gain;
                    best_worst = worst_gain;
                    best_parameters = {radius, spatial_sigma, 0.035F, 0.025F, multiplier};
                }
            }
        }
    }
    std::cout << std::fixed << std::setprecision(4)
              << "Best tuning-set synthetic configuration\n"
              << "radius=" << best_parameters.radius << '\n'
              << "spatial_sigma=" << best_parameters.spatial_sigma << '\n'
              << "range_multiplier=" << best_parameters.range_multiplier << '\n'
              << "mean_psnr_gain_db=" << best_average << '\n'
              << "worst_case_gain_db=" << best_worst << '\n';

    for (std::size_t profile_id = 0; profile_id < profiles.size(); ++profile_id) {
        double baseline_sum = 0.0;
        double restored_sum = 0.0;
        for (std::uint32_t variant = 0; variant < 6; ++variant) {
            const auto rgb = rawforge::create_test_scene(320, 240, variant);
            const auto target = rawforge::mosaic_bayer_rggb(rgb);
            const auto clean_quad = rawforge::mosaic_quad_rggb(rgb);
            const auto profile = profiles[profile_id];
            const auto noisy = rawforge::add_sensor_noise(
                clean_quad,
                profile.read_sigma,
                profile.shot_scale,
                2026U + variant * 17U + static_cast<std::uint32_t>(profile_id)
            );
            const auto baseline = rawforge::remosaic_quad_to_bayer(noisy);
            const rawforge::DenoiseParameters parameters{
                best_parameters.radius,
                best_parameters.spatial_sigma,
                profile.read_sigma,
                profile.shot_scale,
                best_parameters.range_multiplier,
            };
            const auto restored = rawforge::joint_remosaic_denoise(noisy, parameters);
            baseline_sum += rawforge::psnr(target, baseline);
            restored_sum += rawforge::psnr(target, restored);
        }
        std::cout << "profile_" << profile_id << "_baseline_psnr_db=" << baseline_sum / 6.0
                  << '\n'
                  << "profile_" << profile_id << "_restored_psnr_db=" << restored_sum / 6.0
                  << '\n'
                  << "profile_" << profile_id << "_gain_db="
                  << (restored_sum - baseline_sum) / 6.0 << '\n';
    }
}
