#ifndef WFC_UTILS_ARRAY2D_HPP_
#define WFC_UTILS_ARRAY2D_HPP_

#include <vector>

#include "assert.h"

using std::size_t;
using std::vector;

/**
 * Represent a 2D array.
 * The 2D array is stored in a single array, to improve cache usage.
 */
template <typename T>
class Array2D {
   public:
    /**
     * MY and MX of the 2D array.
     */
    size_t MX;
    size_t MY;

    /**
     * The array containing the data of the 2D array.
     */
    vector<T> data;

    /**
     * Build a 2D array given its MY and MX.
     * All the array elements are initialized to default value.
     */
    Array2D(size_t MX, size_t MY) noexcept : MX(MX), MY(MY), data(MX * MY) {}

    /**
     * Build a 2D array given its MY and MX.
     * All the array elements are initialized to value.
     */
    Array2D(size_t MX, size_t MY, T value) noexcept
        : MX(MX), MY(MY), data(MX * MY, value) {}

    inline T get(size_t x, size_t y) const noexcept {
        assert(x < MX && y < MY);
        return static_cast<T>(data[x + y * MX]);
    }

    inline void set(size_t x, size_t y, T value) { data[x + y * MX] = value; }

    inline void fill(T value) { std::fill(data.begin(), data.end(), value); }

    /** Return the current 2D array reflected along the x axis */
    inline Array2D<T> reflected() const noexcept {
        Array2D<T> result = Array2D<T>(MX, MY);
        for (size_t y = 0; y < MY; y++) {
            for (size_t x = 0; x < MX; x++) {
                result.set(x, y, get(MX - 1 - x, y));
            }
        }
        return result;
    }

    /** Return the current 2D array rotated 90 deg anticlockwise */
    inline Array2D<T> rotated() const noexcept {
        Array2D<T> result = Array2D<T>(MX, MY);
        for (std::size_t y = 0; y < MX; y++) {
            for (std::size_t x = 0; x < MY; x++) {
                result.set(x, y, get(MX - 1 - y, x));
            }
        }
        return result;
    }

    /** Check if two 2D arrays are equals */
    inline bool operator==(const Array2D<T> &a) const noexcept {
        if (MX != a.MX || MY != a.MY) return false;
        return data == a.data;
    }
};

/**
 * Hash function.
 */
namespace std {
template <typename T>
class hash<Array2D<T>> {
   public:
    std::size_t operator()(const Array2D<T> &a) const noexcept {
        std::size_t seed = a.data.size();
        for (const T &i : a.data) {
            seed ^= hash<T>()(i) + (std::size_t)0x9e3779b9 + (seed << 6) +
                    (seed >> 2);
        }
        return seed;
    }
};
}  // namespace std

#endif  // WFC_UTILS_ARRAY2D_HPP_
