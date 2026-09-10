#pragma once

#include "rawforge/image.hpp"

namespace rawforge {

[[nodiscard]] double mse(const Image<float>& reference, const Image<float>& prediction);
[[nodiscard]] double psnr(const Image<float>& reference, const Image<float>& prediction);

}  // namespace rawforge

