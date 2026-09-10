#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

#include "rawforge/metrics.hpp"
#include "rawforge/pipeline.hpp"
#include "rawforge/synthetic.hpp"

int main() {
    constexpr int repetitions = 3;
    std::cout << "width,height,megapixels,baseline_ms,joint_ms,joint_ms_per_mp,psnr_gain_db\n";
    for (const auto [width, height] : std::vector<std::pair<std::size_t, std::size_t>>{
             {320, 240}, {640, 480}, {1280, 720}}) {
        const auto rgb = rawforge::create_test_scene(width, height);
        const auto target = rawforge::mosaic_bayer_rggb(rgb);
        const auto noisy = rawforge::add_sensor_noise(rawforge::mosaic_quad_rggb(rgb));
        double baseline_total = 0.0;
        double joint_total = 0.0;
        rawforge::Image<float> baseline;
        rawforge::Image<float> restored;
        for (int repetition = 0; repetition < repetitions; ++repetition) {
            auto started = std::chrono::steady_clock::now();
            baseline = rawforge::remosaic_quad_to_bayer(noisy);
            auto finished = std::chrono::steady_clock::now();
            baseline_total += std::chrono::duration<double, std::milli>(finished - started).count();

            started = std::chrono::steady_clock::now();
            restored = rawforge::joint_remosaic_denoise(noisy);
            finished = std::chrono::steady_clock::now();
            joint_total += std::chrono::duration<double, std::milli>(finished - started).count();
        }
        const double megapixels = static_cast<double>(width * height) / 1'000'000.0;
        const double baseline_ms = baseline_total / repetitions;
        const double joint_ms = joint_total / repetitions;
        std::cout << width << ',' << height << ',' << std::fixed << std::setprecision(4)
                  << megapixels << ',' << baseline_ms << ',' << joint_ms << ','
                  << joint_ms / megapixels << ','
                  << rawforge::psnr(target, restored) - rawforge::psnr(target, baseline) << '\n';
    }
}

