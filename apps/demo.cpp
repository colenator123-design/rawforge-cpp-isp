#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "rawforge/io.hpp"
#include "rawforge/metrics.hpp"
#include "rawforge/pipeline.hpp"
#include "rawforge/synthetic.hpp"

int main(int argc, char** argv) {
    const std::filesystem::path output_directory = argc > 1 ? argv[1] : "outputs/demo";
    constexpr std::size_t width = 640;
    constexpr std::size_t height = 480;

    const auto rgb = rawforge::create_test_scene(width, height);
    const auto target_bayer = rawforge::mosaic_bayer_rggb(rgb);
    const auto clean_quad = rawforge::mosaic_quad_rggb(rgb);
    const auto noisy_quad = rawforge::add_sensor_noise(clean_quad);

    const auto baseline_started = std::chrono::steady_clock::now();
    const auto baseline_bayer = rawforge::remosaic_quad_to_bayer(noisy_quad);
    const auto baseline_finished = std::chrono::steady_clock::now();

    const auto optimized_started = std::chrono::steady_clock::now();
    const auto restored_bayer = rawforge::joint_remosaic_denoise(noisy_quad);
    const auto optimized_finished = std::chrono::steady_clock::now();

    const double baseline_psnr = rawforge::psnr(target_bayer, baseline_bayer);
    const double restored_psnr = rawforge::psnr(target_bayer, restored_bayer);
    const double baseline_ms =
        std::chrono::duration<double, std::milli>(baseline_finished - baseline_started).count();
    const double optimized_ms =
        std::chrono::duration<double, std::milli>(optimized_finished - optimized_started).count();

    rawforge::write_ppm(output_directory / "00_clean_scene.ppm", rgb);
    rawforge::write_ppm(
        output_directory / "01_noisy_quad_preview.ppm",
        rawforge::demosaic_bayer_bilinear(noisy_quad)
    );
    rawforge::write_ppm(
        output_directory / "02_baseline_preview.ppm",
        rawforge::demosaic_bayer_bilinear(baseline_bayer)
    );
    rawforge::write_ppm(
        output_directory / "03_restored_preview.ppm",
        rawforge::demosaic_bayer_bilinear(restored_bayer)
    );
    rawforge::write_pgm(output_directory / "noisy_quad_raw.pgm", noisy_quad);
    rawforge::write_pgm(output_directory / "target_bayer_raw.pgm", target_bayer);
    rawforge::write_pgm(output_directory / "restored_bayer_raw.pgm", restored_bayer);

    std::ofstream metrics(output_directory / "metrics.csv");
    metrics << "metric,value\n"
            << "width," << width << '\n'
            << "height," << height << '\n'
            << "baseline_psnr_db," << baseline_psnr << '\n'
            << "restored_psnr_db," << restored_psnr << '\n'
            << "psnr_gain_db," << restored_psnr - baseline_psnr << '\n'
            << "baseline_ms," << baseline_ms << '\n'
            << "joint_pipeline_ms," << optimized_ms << '\n';

    std::cout << std::fixed << std::setprecision(3)
              << "RawForge synthetic Quad-Bayer benchmark\n"
              << "Resolution: " << width << 'x' << height << '\n'
              << "Remosaic baseline: " << baseline_psnr << " dB, " << baseline_ms << " ms\n"
              << "Joint denoise + remosaic: " << restored_psnr << " dB, " << optimized_ms
              << " ms\n"
              << "PSNR gain over baseline: " << restored_psnr - baseline_psnr << " dB\n"
              << "Outputs: " << output_directory << '\n';
    return restored_psnr > baseline_psnr ? 0 : 1;
}
