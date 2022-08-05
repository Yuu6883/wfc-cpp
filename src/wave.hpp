#ifndef WFC_WAVE_HPP_
#define WFC_WAVE_HPP_

#include <limits>
#include <random>
#include <vector>

using std::vector;

#include "utils/array_2d.hpp"
#include "utils/array_3d.hpp"

/**
 * Contains the pattern possibilities in every cell.
 * Also contains information about cell entropy (if shannon is true).
 */
class Wave {
   private:
    vector<double>& weights;
    vector<double>& wLogW;

    struct ShannonEntropy {
        double wSum;      // The sum of p'(pattern).
        double wSumLogW;  // The sum of p'(pattern) * log(p'(pattern)).
        double entropy;   // The entropy of the cell.
    };

    struct CountOnly {
        uint64_t nb_patterns;  // The number of patterns present
    };

    /** Bitmap and counter of pattern compatbility */
    Array2D<bool> data;
    Array3D<uint8_t> compatible;

   public:
    static inline const uint8_t opposite[] = {2, 3, 0, 1, 5, 4};

    struct Propagator {
        struct Entry {
            uint32_t offset;
            uint32_t length;
        };

        Array2D<Entry> table;
        vector<uint16_t> flat;

        Propagator() : table(0, 0){};
    };

    /** Type of heuristic used to choose next unobserved node **/
    enum class Heuristic { Entropy, MRV, Scanline };
    // Counter
    vector<uint16_t> counts;
    /** Memoisation for computating entropy */
    vector<ShannonEntropy> memoisations;

    /** L = total elements in the grid */
    const size_t L, P, D;
    const Heuristic heuristic;

    /** Initialize the wave with every cell being able to have every pattern */
    Wave(size_t L, size_t P, size_t D, vector<double>& weights,
         vector<double>& wLogW, Heuristic heuristic) noexcept
        : L(L),
          P(P),
          D(D),
          heuristic(heuristic),
          data(P, L),
          counts(L),
          memoisations(0),
          compatible(D, P, L),
          weights(weights),
          wLogW(wLogW) {}

    inline void init(const Propagator& propagator, double wSum, double wSumLogW,
                     double e0) {
        data.fill(true);

        for (size_t i = 0; i < L; i++) {
            for (size_t p = 0; p < P; p++) {
                for (size_t d = 0; d < D; d++) {
                    compatible.set(d, p, i,
                                   propagator.table.get(opposite[d], p).length);
                }
            }
        }

        std::fill(counts.begin(), counts.end(), P);

        if (heuristic == Heuristic::Entropy) {
            memoisations = vector<ShannonEntropy>(
                L, {.wSum = wSum, .wSumLogW = wSumLogW, .entropy = e0});
        } else if (heuristic == Heuristic::Scanline) {
            scanCursor = 0;
        }
    }

    /** Return true if pattern can be placed in cell index */
    inline bool get(size_t index, size_t pattern) const noexcept {
        return data.get(pattern, index);
    }

    /** Ban pattern in cell index */
    inline void ban(size_t index, size_t pattern) noexcept {
        data.set(pattern, index, false);
        for (size_t d = 0; d < D; d++) {
            compatible.set(d, pattern, index, 0);
        }

        counts[index]--;

        if (heuristic == Heuristic::Entropy) {
            double sum = memoisations[index].wSum;
            memoisations[index].entropy +=
                memoisations[index].wSumLogW / sum - log(sum);

            memoisations[index].wSum -= weights[pattern];
            memoisations[index].wSumLogW -= wLogW[pattern];

            sum = memoisations[index].wSum;
            memoisations[index].entropy -=
                memoisations[index].wSumLogW / sum - log(sum);
        }
    }

    size_t scanCursor = 0;
    /**
     * Return the index of the cell with lowest entropy different of 0.
     * If there is a contradiction in the wave, return -2.
     * If every cell is decided, return -1.
     */
    template <typename RNG>
    inline int observe_next(size_t MX, size_t MY, size_t MZ, size_t N,
                            bool periodic, RNG& gen) noexcept {
        if (heuristic == Heuristic::Scanline) {
            for (size_t i = scanCursor; i < L; i++) {
                size_t x = i % MX;
                size_t y = (i % (MX * MY)) / MX;
                size_t z = i / (MX * MY);

                if (!periodic && (x + N > MX || y + N > MY || z + 1 > MZ))
                    continue;

                if (counts[i] > 1) {
                    scanCursor = i + 1;
                    return i;
                }
            }
            return -1;
        }

        double min = std::numeric_limits<double>::max();
        int argmin = -1;

        std::uniform_real_distribution<double> gen_noise(0.0, 1e-6);

        for (size_t z = 0; z < MZ; z++) {
            for (size_t y = 0; y < MY; y++) {
                for (size_t x = 0; x < MX; x++) {
                    if (!periodic && (x + N > MX || y + N > MY || z + 1 > MZ))
                        continue;
                    size_t i = x + y * MX + z * MX * MY;
                    uint32_t remainingValues = counts[i];
                    double entropy = heuristic == Heuristic::Entropy
                                         ? memoisations[i].entropy
                                         : remainingValues;
                    if (remainingValues > 1 && entropy <= min) {
                        double noise = gen_noise(gen);
                        if (entropy + noise < min) {
                            min = entropy + noise;
                            argmin = i;
                        }
                    }
                }
            }
        }
        return argmin;
    };

    inline int32_t decre_comp(uint8_t dir, size_t pattern, size_t index) {
        auto& c = this->compatible.ref(dir, pattern, index);
        // Hopefully save 1 unnecessary store op
        return (c <= 0) ? -1 : --c;
    }

    inline size_t bytes() {
        return compatible.data.size() * sizeof(compatible.data[0]) +
               counts.size() * sizeof(counts[0]);
    }
};

#endif  // WFC_WAVE_HPP_