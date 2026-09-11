#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "rawforge/metrics.hpp"
#include "rawforge/pipeline.hpp"
#include "rawforge/synthetic.hpp"

struct NoiseProfile {
    std::string_view name;
    float read_sigma;
    float shot_scale;
};

int main() {
    constexpr std::array profiles = {
        NoiseProfile{"low", 0.015F, 0.010F},
        NoiseProfile{"medium", 0.035F, 0.025F},
        NoiseProfile{"high", 0.060F, 0.040F},
    };
    constexpr std::uint32_t scene_count = 8;
    std::cout << "profile,scenes,baseline_psnr_db,restored_psnr_db,psnr_gain_db,"
                 "baseline_ssim,restored_ssim,ssim_gain\n";
    for (std::size_t profile_id = 0; profile_id < profiles.size(); ++profile_id) {
        const auto profile = profiles[profile_id];
        double baseline_psnr = 0.0;
        double restored_psnr = 0.0;
        double baseline_ssim = 0.0;
        double restored_ssim = 0.0;
        for (std::uint32_t variant = 6; variant < 6 + scene_count; ++variant) {
            const auto rgb = rawforge::create_test_scene(320, 240, variant);
            const auto target = rawforge::mosaic_bayer_rggb(rgb);
            const auto noisy = rawforge::add_sensor_noise(
                rawforge::mosaic_quad_rggb(rgb),
                profile.read_sigma,
                profile.shot_scale,
                2026U + variant * 17U + static_cast<std::uint32_t>(profile_id)
            );
            const auto baseline = rawforge::remosaic_quad_to_bayer(noisy);
            rawforge::DenoiseParameters parameters;
            parameters.read_noise_sigma = profile.read_sigma;
            parameters.shot_noise_scale = profile.shot_scale;
            const auto restored = rawforge::joint_remosaic_denoise(noisy, parameters);
            baseline_psnr += rawforge::psnr(target, baseline);
            restored_psnr += rawforge::psnr(target, restored);
            baseline_ssim += rawforge::ssim(target, baseline);
            restored_ssim += rawforge::ssim(target, restored);
        }
        baseline_psnr /= scene_count;
        restored_psnr /= scene_count;
        baseline_ssim /= scene_count;
        restored_ssim /= scene_count;
        std::cout << profile.name << ',' << scene_count << ',' << std::fixed << std::setprecision(5)
                  << baseline_psnr << ',' << restored_psnr << ','
                  << restored_psnr - baseline_psnr << ',' << baseline_ssim << ','
                  << restored_ssim << ',' << restored_ssim - baseline_ssim << '\n';
    }
}
