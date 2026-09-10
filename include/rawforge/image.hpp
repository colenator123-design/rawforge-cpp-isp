#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace rawforge {

template <typename T>
class Image {
public:
    Image() = default;

    Image(std::size_t width, std::size_t height, std::size_t channels = 1, T value = {})
        : width_(width), height_(height), channels_(channels), data_(width * height * channels, value) {
        if (width == 0 || height == 0 || channels == 0) {
            throw std::invalid_argument("Image dimensions and channels must be positive");
        }
    }

    [[nodiscard]] std::size_t width() const noexcept { return width_; }
    [[nodiscard]] std::size_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t channels() const noexcept { return channels_; }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }

    T& operator()(std::size_t x, std::size_t y, std::size_t channel = 0) {
        return data_.at((y * width_ + x) * channels_ + channel);
    }

    const T& operator()(std::size_t x, std::size_t y, std::size_t channel = 0) const {
        return data_.at((y * width_ + x) * channels_ + channel);
    }

    [[nodiscard]] const std::vector<T>& data() const noexcept { return data_; }
    [[nodiscard]] std::vector<T>& data() noexcept { return data_; }

private:
    std::size_t width_ = 0;
    std::size_t height_ = 0;
    std::size_t channels_ = 0;
    std::vector<T> data_;
};

inline int clamp_coordinate(int value, int upper_bound) {
    return std::clamp(value, 0, upper_bound - 1);
}

}  // namespace rawforge

