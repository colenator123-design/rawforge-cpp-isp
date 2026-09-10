#include "rawforge/metrics.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace rawforge {

double mse(const Image<float>& reference, const Image<float>& prediction) {
    if (reference.width() != prediction.width() || reference.height() != prediction.height() ||
        reference.channels() != prediction.channels()) {
        throw std::invalid_argument("Metric inputs must have identical dimensions");
    }
    double squared_error = 0.0;
    for (std::size_t index = 0; index < reference.size(); ++index) {
        const double difference =
            static_cast<double>(reference.data()[index]) - prediction.data()[index];
        squared_error += difference * difference;
    }
    return squared_error / static_cast<double>(reference.size());
}

double psnr(const Image<float>& reference, const Image<float>& prediction) {
    const double error = mse(reference, prediction);
    if (error <= std::numeric_limits<double>::epsilon()) {
        return std::numeric_limits<double>::infinity();
    }
    return 10.0 * std::log10(1.0 / error);
}

}  // namespace rawforge

