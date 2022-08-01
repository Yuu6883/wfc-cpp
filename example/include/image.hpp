#ifndef WFC_UTILS_IMAGE_HPP_
#define WFC_UTILS_IMAGE_HPP_

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <array>
#include <optional>
#include "external/stb_image.h"
#include "external/stb_image_write.h"
#include "utils/array_2d.hpp"

using std::array;

/**
 * Read an image. Returns nullopt if there was an error.
 */
std::optional<Array2D<uint32_t>> read_image(
    const std::string& file_path) noexcept {
    int width;
    int height;
    int num_components;

    uint8_t* data =
        stbi_load(file_path.c_str(), &width, &height, &num_components, 3);

    if (!data) return std::nullopt;

    Array2D<uint32_t> m = Array2D<uint32_t>(width, height);
    for (auto i = 0; i < height; i++) {
        for (auto j = 0; j < width; j++) {
            size_t index = 3 * (i * width + j);
            uint8_t r = data[index + 0];
            uint8_t g = data[index + 1];
            uint8_t b = data[index + 2];

            m.data[i * width + j] = (r << 16) | (g << 8) | b;
        }
    }

    free(data);
    return m;
}

/**
 * Write an image in the png format.
 */
void write_image_png(const std::string& file_path,
                     const Array2D<array<uint8_t, 3>>& m) noexcept {
    stbi_write_png(file_path.c_str(), m.MX, m.MY, 3,
                   (const uint8_t*)m.data.data(), 0);
}

#endif  // WFC_UTILS_IMAGE_HPP_
