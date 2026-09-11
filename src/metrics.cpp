#include "rawforge/metrics.hpp"

#include <algorithm>
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

double ssim(const Image<float>& reference, const Image<float>& prediction) {
    if (reference.width() != prediction.width() || reference.height() != prediction.height() ||
        reference.channels() != prediction.channels()) {
        throw std::invalid_argument("Metric inputs must have identical dimensions");
    }
    constexpr std::size_t window_size = 8;
    constexpr double c1 = 0.01 * 0.01;
    constexpr double c2 = 0.03 * 0.03;
    double score_sum = 0.0;
    std::size_t window_count = 0;
    for (std::size_t y0 = 0; y0 < reference.height(); y0 += window_size) {
        for (std::size_t x0 = 0; x0 < reference.width(); x0 += window_size) {
            const std::size_t x1 = std::min(x0 + window_size, reference.width());
            const std::size_t y1 = std::min(y0 + window_size, reference.height());
            const double count = static_cast<double>((x1 - x0) * (y1 - y0));
            double mean_reference = 0.0;
            double mean_prediction = 0.0;
            for (std::size_t y = y0; y < y1; ++y) {
                for (std::size_t x = x0; x < x1; ++x) {
                    mean_reference += reference(x, y);
                    mean_prediction += prediction(x, y);
                }
            }
            mean_reference /= count;
            mean_prediction /= count;
            double variance_reference = 0.0;
            double variance_prediction = 0.0;
            double covariance = 0.0;
            for (std::size_t y = y0; y < y1; ++y) {
                for (std::size_t x = x0; x < x1; ++x) {
                    const double centered_reference = reference(x, y) - mean_reference;
                    const double centered_prediction = prediction(x, y) - mean_prediction;
                    variance_reference += centered_reference * centered_reference;
                    variance_prediction += centered_prediction * centered_prediction;
                    covariance += centered_reference * centered_prediction;
                }
            }
            variance_reference /= count;
            variance_prediction /= count;
            covariance /= count;
            const double numerator =
                (2.0 * mean_reference * mean_prediction + c1) * (2.0 * covariance + c2);
            const double denominator =
                (mean_reference * mean_reference + mean_prediction * mean_prediction + c1) *
                (variance_reference + variance_prediction + c2);
            score_sum += numerator / denominator;
            ++window_count;
        }
    }
    return score_sum / static_cast<double>(window_count);
}

}  // namespace rawforge
