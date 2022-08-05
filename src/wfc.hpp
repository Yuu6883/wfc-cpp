#ifndef WFC_WFC_HPP_
#define WFC_WFC_HPP_

#include <optional>
#include <random>

#include "utils/array_2d.hpp"
#include "utils/xoshiro256ss.hpp"
#include "wave.hpp"

using std::optional;
using std::vector;

#ifdef WIN32
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

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
   protected:
    using Heuristic = Wave::Heuristic;
    using Propagator = Wave::Propagator;

   private:
    vector<double> wLogW;
    vector<double> distribution;

    double wSum, wSumLogW, e0;

    struct BanItem {
        uint16_t index;
        uint16_t pattern;
    };

    vector<BanItem> stack;
    size_t stack_len = 0;

    /** The propagator, used to propagate the information in the wave */
    Propagator propagator;
    // Propagator propagator_copy;

   protected:
    inline constexpr static int8_t DX[] = {-1, 0, 1, 0, 0, 0};
    inline constexpr static int8_t DY[] = {0, 1, 0, -1, 0, 0};
    inline constexpr static int8_t DZ[] = {0, 0, 0, 0, 1, -1};

    size_t P = 0;
    const size_t MX, MY, MZ, N;

    const bool periodic;
    const Heuristic heuristic;

    /** Distribution of the patterns as given in input */
    vector<double> weights;

    /** The wave, indicating which patterns can be put in which cell */
    Wave* wave = nullptr;

    virtual void init() noexcept = 0;
    virtual bool clear() noexcept = 0;

    template <typename CB>
    inline void from_dense(const CB& agree) {
        propagator.flat.clear();
        propagator.table = Array2D<Propagator::Entry>(4, P);

        uint32_t offset = 0;
        uint32_t dense = 0;

        for (uint32_t p1 = 0; p1 < P; p1++) {
#pragma unroll
            for (uint8_t d = 0; d < 4; d++) {
                Propagator::Entry entry{};
                entry.offset = offset;

                for (uint32_t p2 = 0; p2 < P; p2++) {
                    if (agree(p1, p2, d)) {
                        offset++;
                        entry.length++;
                        propagator.flat.push_back(p2);
                    }
                }

                propagator.table.set(d, p1, entry);
                dense += entry.length;
            }
        }

        propagator.flat.shrink_to_fit();

        // density = 100% - sparsity
        fprintf(stderr, "Propagator density: %.2f%%\n",
                100.f * dense / (P * P * 4));
    }

    /** Initialize wave */
    void post_init() noexcept {
        size_t L = MX * MY * MZ;
        size_t D = propagator.table.MX;
        stack.reserve(L * P);
        stack.resize(L * P);
        stack_len = 0;

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

        fprintf(stderr, "P = %lu, D = %lu, L = %lu\n", P, D, L);

        auto b = bytes();
        if (b > 1024 * 1024) {
            fprintf(stderr, "memory usage: %.2fmb\n", b / 1024.f / 1024.f);
        } else {
            fprintf(stderr, "memory usage: %.2fkb\n", b / 1024.f);
        }
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
          heuristic(heuristic){};

    virtual ~WFC() {
        if (wave) delete wave;
    }

    /** Run the algorithm, and return if it succeeded */
    bool run(uint32_t seed, int32_t limit = -1) noexcept {
        xoshiro256ss rng(seed);

        if (!wave) {
            init();
            post_init();

            // propagator_copy = propagator;
        } else {
            // propagator = propagator_copy;
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
            } else
                break;
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

        for (uint16_t p = 0; p < P; p++) {
            if (wave->get(index, p) != (p == collapsed)) ban(index, p);
        }
    };

    inline void ban(uint16_t index, uint16_t p) {
        wave->ban(index, p);
        stack[stack_len++] = {.index = index, .pattern = p};
    }

    /** Propagate the state */
    template <typename RNG>
    NOINLINE bool propagate(RNG& rng) noexcept {
        while (stack_len) {
            // Random pop experiment, yields same result
            /*
            std::uniform_int_distribution<size_t> index_range(0, stack.size() -
            1); size_t index = index_range(rng); auto item = stack[index];

            stack[index] = stack.back();
            */
            const auto item = stack[stack_len - 1];
            stack_len--;

            const size_t x1 = item.index % MX;
            const size_t y1 = item.index / MX;

            // const size_t y1 = (item.index % (MX * MY)) / MX;
            // const size_t z1 = item.index / (MX * MY);

            for (int d = 0; d < wave->D; d++) {
                const int dx = DX[d], dy = DY[d];  // , dz = DZ[d];

                int x2 = x1 + dx, y2 = y1 + dy;  // , z2 = z1 + dz;
                if (!periodic && (x2 < 0 || y2 < 0 ||
                                  // z2 < 0 ||
                                  x2 + N > MX || y2 + N > MY))
                    // || z2 + 1 > MZ))
                    continue;

                x2 = (x2 + MX) % MX;
                y2 = (y2 + MY) % MY;
                // z2 = (z2 + MZ) % MZ;

                const size_t i2 = x2 + y2 * MX;  // +z2 * MX * MY;

                const auto entry = propagator.table.get(d, item.pattern);

                for (size_t pattern_index = entry.offset;
                     pattern_index < entry.offset + entry.length;
                     pattern_index++) {
                    const auto p2 = propagator.flat[pattern_index];

                    if (wave->decre_comp(d, p2, i2) == 0) {
                        ban(i2, p2);
                    }
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

    inline size_t bytes() {
        return wave->bytes() +
               propagator.table.data.size() * sizeof(Propagator::Entry) +
               propagator.flat.size() * sizeof(propagator.flat[0]) +
               stack.capacity() * sizeof(stack[0]) +
               weights.size() * sizeof(weights[0]) +
               wLogW.size() * sizeof(wLogW[0]);
    }
};

#endif  // WFC_WFC_HPP_
