#include "rawforge/synthetic.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

#include "rawforge/cfa.hpp"

namespace rawforge {
namespace {

float smooth_step(const float edge0, const float edge1, const float value) {
    const float normalized = std::clamp((value - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return normalized * normalized * (3.0F - 2.0F * normalized);
}

Image<float> mosaic(const Image<float>& rgb, const bool quad) {
    if (rgb.channels() != 3) {
        throw std::invalid_argument("Mosaic input must be RGB");
    }
    Image<float> raw(rgb.width(), rgb.height());
    for (std::size_t y = 0; y < rgb.height(); ++y) {
        for (std::size_t x = 0; x < rgb.width(); ++x) {
            const Color color = quad ? quad_rggb_color(x, y) : bayer_rggb_color(x, y);
            raw(x, y) = rgb(x, y, static_cast<std::size_t>(color));
        }
    }
    return raw;
}

}  // namespace

Image<float> create_test_scene(const std::size_t width, const std::size_t height) {
    Image<float> image(width, height, 3);
    const float scale = static_cast<float>(std::min(width, height));
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const float fx = static_cast<float>(x) / static_cast<float>(width - 1U);
            const float fy = static_cast<float>(y) / static_cast<float>(height - 1U);
            float red = 0.08F + 0.72F * fx;
            float green = 0.08F + 0.65F * fy;
            float blue = 0.12F + 0.45F * (1.0F - fx);

            const float dx = static_cast<float>(x) - 0.72F * static_cast<float>(width);
            const float dy = static_cast<float>(y) - 0.30F * static_cast<float>(height);
            const float circle = 1.0F - smooth_step(0.18F * scale, 0.19F * scale, std::hypot(dx, dy));
            red = red * (1.0F - circle) + 0.95F * circle;
            green = green * (1.0F - circle) + 0.18F * circle;
            blue = blue * (1.0F - circle) + 0.12F * circle;

            if (x > width / 12U && x < width * 5U / 12U && y > height / 5U &&
                y < height * 4U / 5U) {
                const bool checker = ((x / 10U) + (y / 10U)) % 2U == 0U;
                const float detail = checker ? 0.82F : 0.18F;
                red = 0.25F + 0.65F * detail;
                green = 0.15F + 0.55F * (1.0F - detail);
                blue = 0.20F + 0.60F * detail;
            }

            const bool fine_lines = x > width / 2U && y > height * 3U / 5U &&
                                    ((x / 3U) % 2U == 0U || (y / 3U) % 2U == 0U);
            if (fine_lines) {
                red = 0.92F;
                green = 0.92F;
                blue = 0.92F;
            }
            image(x, y, 0) = std::clamp(red, 0.0F, 1.0F);
            image(x, y, 1) = std::clamp(green, 0.0F, 1.0F);
            image(x, y, 2) = std::clamp(blue, 0.0F, 1.0F);
        }
    }
    return image;
}

Image<float> mosaic_quad_rggb(const Image<float>& rgb) { return mosaic(rgb, true); }

Image<float> mosaic_bayer_rggb(const Image<float>& rgb) { return mosaic(rgb, false); }

Image<float> add_sensor_noise(
    const Image<float>& clean,
    const float read_sigma,
    const float shot_scale,
    const std::uint32_t seed
) {
    std::mt19937 generator(seed);
    std::normal_distribution<float> normal(0.0F, 1.0F);
    Image<float> noisy(clean.width(), clean.height(), clean.channels());
    for (std::size_t index = 0; index < clean.size(); ++index) {
        const float signal = clean.data()[index];
        const float sigma = read_sigma + shot_scale * std::sqrt(std::max(signal, 0.0F));
        noisy.data()[index] = std::clamp(signal + sigma * normal(generator), 0.0F, 1.0F);
    }
    return noisy;
}

}  // namespace rawforge

