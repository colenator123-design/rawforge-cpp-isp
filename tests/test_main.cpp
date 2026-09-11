#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "rawforge/cfa.hpp"
#include "rawforge/metrics.hpp"
#include "rawforge/pipeline.hpp"
#include "rawforge/synthetic.hpp"

namespace {

int failures = 0;

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void test_cfa_patterns() {
    using rawforge::Color;
    expect(rawforge::quad_rggb_color(0, 0) == Color::Red, "Quad top-left is red");
    expect(rawforge::quad_rggb_color(1, 1) == Color::Red, "Quad red uses a 2x2 block");
    expect(rawforge::quad_rggb_color(2, 0) == Color::Green, "Quad top-right is green");
    expect(rawforge::quad_rggb_color(0, 2) == Color::Green, "Quad bottom-left is green");
    expect(rawforge::quad_rggb_color(3, 3) == Color::Blue, "Quad bottom-right is blue");
    expect(rawforge::bayer_rggb_color(0, 0) == Color::Red, "Bayer top-left is red");
    expect(rawforge::bayer_rggb_color(1, 0) == Color::Green, "Bayer top-right is green");
    expect(rawforge::bayer_rggb_color(1, 1) == Color::Blue, "Bayer bottom-right is blue");
}

void test_constant_is_preserved() {
    rawforge::Image<float> constant(32, 24, 1, 0.42F);
    const auto result = rawforge::joint_remosaic_denoise(constant);
    for (const float value : result.data()) {
        expect(std::abs(value - 0.42F) < 1.0e-5F, "Constant RAW value is preserved");
    }
}

void test_denoising_improves_synthetic_psnr() {
    const auto rgb = rawforge::create_test_scene(160, 120);
    const auto target = rawforge::mosaic_bayer_rggb(rgb);
    const auto noisy = rawforge::add_sensor_noise(rawforge::mosaic_quad_rggb(rgb));
    const auto baseline = rawforge::remosaic_quad_to_bayer(noisy);
    const auto restored = rawforge::joint_remosaic_denoise(noisy);
    expect(rawforge::psnr(target, restored) > rawforge::psnr(target, baseline),
           "Joint denoise and remosaic improves PSNR");
}

void test_deterministic_noise() {
    const auto raw = rawforge::mosaic_quad_rggb(rawforge::create_test_scene(32, 24));
    const auto first = rawforge::add_sensor_noise(raw, 0.03F, 0.02F, 7);
    const auto second = rawforge::add_sensor_noise(raw, 0.03F, 0.02F, 7);
    expect(rawforge::mse(first, second) == 0.0, "Fixed seed produces identical sensor noise");
}

void test_identical_images_have_perfect_metrics() {
    const auto image = rawforge::mosaic_quad_rggb(rawforge::create_test_scene(32, 24));
    expect(rawforge::mse(image, image) == 0.0, "Identical images have zero MSE");
    expect(std::isinf(rawforge::psnr(image, image)), "Identical images have infinite PSNR");
    expect(std::abs(rawforge::ssim(image, image) - 1.0) < 1.0e-9,
           "Identical images have SSIM one");
}

}  // namespace

int main() {
    try {
        test_cfa_patterns();
        test_constant_is_preserved();
        test_denoising_improves_synthetic_psnr();
        test_deterministic_noise();
        test_identical_images_have_perfect_metrics();
    } catch (const std::exception& error) {
        std::cerr << "Unexpected exception: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All RawForge tests passed\n";
    return EXIT_SUCCESS;
}
