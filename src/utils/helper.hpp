#ifndef WFC_HELPER_HPP_
#define WFC_HELPER_HPP_

#include <functional>
#include <vector>

namespace Helper {

using std::function;
using std::vector;

template <typename T, typename Func>
static inline void pattern(vector<T>& result, size_t N, Func f) {
    result.resize(N * N);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            result[x + y * N] = f(x, y);
        }
    }
}

template <typename T>
static inline void rotated(const vector<T>& in, vector<T>& out, size_t N) {
    pattern(out, N, [&](size_t x, size_t y) { return in[N - 1 - y + x * N]; });
}

template <typename T>
static inline void reflected(const vector<T>& in, vector<T>& out, size_t N) {
    pattern(out, N, [&](size_t x, size_t y) { return in[N - 1 - x + y * N]; });
}

template <typename T, typename Rot, typename Ref, typename Cmp>
static inline vector<T> squareSymmetries(const T& thing, const Rot& rot,
                                         const Ref& ref, const Cmp& comp,
                                         uint8_t subgroup = 0xFF) {
    T things[8];

    things[0] = thing;

    ref(things[1], things[0]);  // b
    rot(things[2], things[0]);  // a
    ref(things[3], things[2]);  // ba
    rot(things[4], things[2]);  // a2
    ref(things[5], things[4]);  // ba2
    rot(things[6], things[4]);  // a3
    ref(things[7], things[6]);  // ba3

    vector<T> result;
    result.reserve(8);

    for (uint8_t i = 0; i < 8; i++) {
        if (!((subgroup >> i) & 1)) continue;

        auto iter = std::find_if(result.begin(), result.end(),
                                 [&](T& s) { return comp(s, things[i]); });

        if (iter == result.end()) {
            result.push_back(things[i]);
        }
    }

    return result;
}

}  // namespace Helper

#endif  // WFC_HELPER_HPP_