#ifndef WFC_WFC_HPP_
#define WFC_WFC_HPP_

#include <optional>
#include <random>

#include "utils/array_2d.hpp"
#include "utils/xoshiro256ss.h"
#include "wave.hpp"

using std::optional;
using std::vector;

namespace {
static int sample(vector<double> weights, double r) {
    double sum = 0;
    for (size_t i = 0; i < weights.size(); i++) sum += weights[i];
    double threshold = r * sum;

    double partialSum = 0;
    for (size_t i = 0; i < weights.size(); i++) {
        partialSum += weights[i];
        if (partialSum >= threshold) return i;
    }
    return 0;
}
}  // namespace

/**
 * Class containing the generic WFC algorithm.
 */
class WFC {
    using Heuristic = Wave::Heuristic;
    using Propagator = Wave::Propagator;

   private:
    vector<double> wLogW;
    vector<double> distribution;

    double wSum, wSumLogW, e0;

    struct BanItem {
        size_t index;
        size_t pattern;
    };

    vector<BanItem> stack;

   protected:
    inline constexpr static int8_t DX[] = {-1, 0, 1, 0, 0, 0};
    inline constexpr static int8_t DY[] = {0, 1, 0, -1, 0, 0};
    inline constexpr static int8_t DZ[] = {0, 0, 0, 0, 1, -1};

    /** Distribution of the patterns as given in input */
    vector<double> weights;

    /** The wave, indicating which patterns can be put in which cell */
    Wave* wave = nullptr;

    /** The propagator, used to propagate the information in the wave */
    Propagator propagator;

    size_t P = 0;
    const size_t MX, MY, MZ, N;

    const bool periodic;
    const Heuristic heuristic;

    virtual void init() noexcept = 0;
    virtual bool clear() noexcept = 0;

    /** Initialize wave */
    void post_init() noexcept {
        size_t L = MX * MY * MZ;
        size_t D = propagator.size();
        stack.reserve(L * P);

        wSum = 0;
        wSumLogW = 0;
        e0 = 0;

        if (heuristic == Heuristic::Entropy) {
            wLogW = vector<double>(P, 0);

            for (size_t i = 0; i < P; i++) {
                wLogW[i] = weights[i] * log(weights[i]);
                wSum += weights[i];
                wSumLogW += wLogW[i];
            }

            e0 = log(wSum) - wSumLogW / wSum;
        }

        distribution = vector<double>(P);

        wave = new Wave(L, P, D, weights, wLogW, heuristic);
    }

   public:
    WFC(uint32_t MX, uint32_t MY, uint32_t MZ, size_t N, bool periodic,
        Heuristic heuristic)
    noexcept
        : MX(MX),
          MY(MY),
          MZ(MZ),
          N(N),
          periodic(periodic),
          heuristic(heuristic) {};

    /** Run the algorithm, and return if it succeeded */
    bool run(uint32_t seed, int32_t limit = -1) noexcept {
        xoshiro256ss rng(seed);

        if (!wave) {
            init();
            post_init();
        }

        wave->init(propagator, wSum, wSumLogW, e0);

        if (clear()) {
            if (!propagate(rng)) return false;
        }

        for (int32_t l = 0; l < limit || limit < 0; l++) {
            int32_t index = wave->observe_next(MX, MY, MZ, N, periodic, rng);
            if (index >= 0) {
                observe(index, rng);
                if (!propagate(rng)) return false;
            } else break;
        }

        return true;
    };

    /** Observe next node */
    template <typename RNG>
    void observe(size_t index, RNG& rng) noexcept {
        for (size_t p = 0; p < P; p++) {
            distribution[p] = wave->get(index, p) ? weights[p] : 0;
        }

        std::uniform_real_distribution<double> next_double(0.0, 1.0);
        size_t collapsed = sample(distribution, next_double(rng));

        for (size_t p = 0; p < P; p++) {
            if (wave->get(index, p) != (p == collapsed)) ban(index, p);
        }
    };

    inline void ban(size_t index, size_t p) {
        wave->ban(index, p);
        stack.push_back({.index = index, .pattern = p});
    }

    /** Propagate the state */
    template <typename RNG>
    bool propagate(RNG& rng) noexcept {

        while (!stack.empty()) {
            // Random pop experiment, yields same result
            /*
            std::uniform_int_distribution<size_t> index_range(0, stack.size() - 1);
            size_t index = index_range(rng);
            auto item = stack[index];

            stack[index] = stack.back();
            */
            auto item = stack.back();
            stack.pop_back();

            size_t x1 = item.index % MX;
            size_t y1 = (item.index % (MX * MY)) / MX;
            size_t z1 = item.index / (MX * MY);

#pragma unroll
            for (int d = 0; d < 6; d++) {
                if (d >= propagator.size()) break;

                int dx = DX[d], dy = DY[d], dz = DZ[d];

                int x2 = x1 + dx, y2 = y1 + dy, z2 = z1 + dz;
                if (!periodic && (x2 < 0 || y2 < 0 || z2 < 0 || x2 + N > MX ||
                                  y2 + N > MY || z2 + 1 > MZ))
                    continue;

                x2 = (x2 + MX) % MX;
                y2 = (y2 + MY) % MY;
                z2 = (z2 + MZ) % MZ;

                size_t i2 = x2 + y2 * MX + z2 * MX * MY;

                auto& patterns = propagator[d][item.pattern];

                for (auto& p2 : patterns) {
                    if (wave->decre_comp(d, p2, i2) == 0) ban(i2, p2);
                }
            }
        }

        return wave->counts[0] > 0;
    };

    template <typename O, typename T>
    inline vector<O> ords(const vector<T>& data, vector<T>& uniques) {
        vector<O> result(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            const T& d = data[i];
            auto iter = std::find(uniques.begin(), uniques.end(), d);

            O ord = std::distance(uniques.begin(), iter);
            if (iter == uniques.end()) {
                ord = static_cast<O>(uniques.size());
                uniques.push_back(d);
            }
            result[i] = ord;
        }
        return result;
    }
};

#endif  // WFC_WFC_HPP_
