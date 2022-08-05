#ifndef WFC_UTILS_ARRAY3D_HPP_
#define WFC_UTILS_ARRAY3D_HPP_

#include <vector>

#include "assert.h"

using std::size_t;
using std::vector;

/**
 * Represent a 3D array.
 * The 3D array is stored in a single array, to improve cache usage.
 */
template <typename T>
class Array3D {
   public:
    /**
     * The dimensions of the 3D array.
     */
    const size_t MX, MY, MZ, MXY;

    /**
     * The array containing the data of the 3D array.
     */
    vector<T> data;

    /**
     * Build a 2D array given its height, width and depth.
     * All the arrays elements are initialized to default value.
     */
    Array3D(size_t MX, size_t MY, size_t MZ) noexcept
        : MX(MX), MY(MY), MZ(MZ), MXY(MX * MY), data(MX * MY * MZ) {}

    /**
     * Build a 2D array given its height, width and depth.
     * All the arrays elements are initialized to value
     */
    Array3D(size_t MX, size_t MY, size_t MZ, T value) noexcept
        : MX(MX), MY(MY), MZ(MZ), MXY(MX * MY), data(MX * MY * MZ, value) {}

    inline T get(size_t x, size_t y, size_t z) const noexcept {
        assert(x < MX && y < MY && z < MZ);
        return data[x + y * MX + z * MXY];
    }

    inline T& ref(size_t x, size_t y, size_t z) noexcept {
        assert(x < MX && y < MY && z < MZ);
        return data[x + y * MX + z * MXY];
    }

    inline void set(size_t x, size_t y, size_t z, T value) noexcept {
        assert(x < MX && y < MY && z < MZ);
        data[x + y * MX + z * MXY] = value;
    }

    inline void fill(T value) { std::fill(data.begin(), data.end(), value); }

    /** Check if two 3D arrays are equals */
    inline bool operator==(const Array3D& a) const noexcept {
        if (MX != a.MX || MY != a.MY || MZ != a.MZ) return false;
        return data == a.data;
    }
};

#endif  // WFC_UTILS_ARRAY3D_HPP_
