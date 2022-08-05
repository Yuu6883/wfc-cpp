#ifndef WFC_OVERLAPPING_WFC_HPP_
#define WFC_OVERLAPPING_WFC_HPP_

#include <algorithm>
#include <array>
#include <unordered_map>
#include <vector>

#include "utils/array_2d.hpp"
#include "utils/helper.hpp"
#include "wfc.hpp"

using std::array;
using std::unordered_map;
using std::vector;

/**
 * Overlapping WFC algorithm.
 */
class OverlappingWFC : public WFC {
   public:
    /**
     * Options needed to use the overlapping wfc.
     */
    struct Options {
        bool periodic_input;   // True if the input is toric.
        bool periodic_output;  // True if the output is toric.

        size_t i_W;
        size_t i_H;
        size_t o_W;  // The height of the output in pixels.
        size_t o_H;  // The width of the output in pixels.

        // The number of symmetries (the order is defined in wfc).
        uint32_t symmetry;

        // The width and height in pixel of the patterns.
        size_t pattern_size;

        // heuristic used to pick next position to observe
        Wave::Heuristic heuristic;

        bool ground;  // True if the ground needs to be set (see init_ground).
    };

   private:
    /**
     * Options needed by the algorithm.
     */
    const Options &options;
    // Input ref
    const Array2D<uint32_t> &input;

    // Patterns
    vector<vector<uint8_t>> patterns;
    // Colors
    vector<uint32_t> colors;

   public:
    /**
     * Constructor used only to call the other constructor with more
     * computed parameters.
     */
    OverlappingWFC(const Options &options, const Array2D<uint32_t> &input)
        : WFC(options.o_W, options.o_H, 1, options.pattern_size,
              options.periodic_output, options.heuristic),
          options(options),
          input(input) {}

   protected:
    void init() noexcept override {
        colors.clear();

        auto sample = ords<uint8_t>(input.data, colors);
        size_t C = colors.size();

        /*
        for (int y = 0; y < input.MY; y++) {
            for (int x = 0; x < input.MX; x++) {
                fprintf(stderr, "%d", sample[x + y * input.MX]);
            }
            fprintf(stderr, "\n");
        }
        */

        size_t W = 1;
        for (int i = 0; i < N * N; i++) W *= C;

        unordered_map<size_t, size_t> wtable;
        vector<size_t> ordering;

        size_t xmax =
            options.periodic_input ? options.i_W : options.i_W - N + 1;
        size_t ymax =
            options.periodic_input ? options.i_H : options.i_H - N + 1;

        vector<uint8_t> temp(N * N);

        for (size_t y = 0; y < ymax; y++) {
            for (size_t x = 0; x < xmax; x++) {
                Helper::pattern(temp, N, [&](size_t dx, size_t dy) {
                    return sample[(x + dx) % options.i_W +
                                  ((y + dy) % options.i_H) * options.i_W];
                });

                auto symmetries = Helper::squareSymmetries(
                    temp,
                    [&](auto &out, const auto &q) {
                        Helper::rotated(q, out, N);
                    },
                    [&](auto &out, const auto &q) {
                        Helper::reflected(q, out, N);
                    },
                    [](auto &, auto &) { return false; },
                    (uint8_t)options.symmetry);

                for (auto &p : symmetries) {
                    size_t ind = 0, power = 1;
                    for (int i = 0; i < p.size(); i++, power *= C)
                        ind += p[p.size() - 1 - i] * power;

                    if (!(wtable[ind]++)) ordering.push_back(ind);
                }
            }
        }

        P = wtable.size();

        auto patternFromIndex = [&](size_t ind, vector<uint8_t> &result) {
            size_t residue = ind, power = W;
            for (size_t i = 0; i < result.size(); i++) {
                power /= C;
                size_t count = 0;
                while (residue >= power) {
                    residue -= power;
                    count++;
                }
                result[i] = static_cast<uint8_t>(count);
            }
        };

        patterns = vector<vector<uint8_t>>(P, vector<uint8_t>(N * N));
        weights.resize(P);

        for (size_t counter = 0; counter < ordering.size(); counter++) {
            size_t w = ordering[counter];
            patternFromIndex(w, patterns[counter]);
            weights[counter] = wtable[w];
        }

        // How compile this into inline version for fixed dx,dy, and N=2,3?
        from_dense([&](uint32_t i1, uint32_t i2, uint8_t d) {
            const int dx = DX[d];
            const int dy = DY[d];

            const vector<uint8_t> &p1 = patterns[i1];
            const vector<uint8_t> &p2 = patterns[i2];
            size_t xmin = dx < 0 ? 0 : dx, xmax = dx < 0 ? dx + N : N;
            size_t ymin = dy < 0 ? 0 : dy, ymax = dy < 0 ? dy + N : N;

            for (size_t y = ymin; y < ymax; y++)
                for (size_t x = xmin; x < xmax; x++)
                    if (p1[x + N * y] != p2[x - dx + N * (y - dy)])
                        return false;
            return true;
        });
    }

    bool clear() noexcept override {
        if (options.ground) {
            for (size_t x = 0; x < MX; x++) {
                for (size_t p = 0; p < P - 1; p++) ban(x + (MY - 1) * MX, p);
                for (size_t y = 0; y < MY - 1; y++) ban(x + y * MX, P - 1);
            }

            return true;
        }
        return false;
    }

   public:
    /**
     * Transform the wave to a valid output (a 2d array of patterns that
     * aren't in contradiction). This function should be used only when all
     * cell of the wave are defined.
     */
    Array2D<array<uint8_t, 3>> get_output() const noexcept {
        Array2D<array<uint8_t, 3>> out(MX, MY);

        bool sus = false;

        for (size_t y = 0; y < MY; y++) {
            int dy = y < MY - N + 1 ? 0 : N - 1;

            for (size_t x = 0; x < MX; x++) {
                int dx = x < MX - N + 1 ? 0 : N - 1;

                int ob = -1;
                for (int p = 0; p < P; p++) {
                    if (wave->get(x - dx + (y - dy) * MX, p)) {
                        ob = p;
                        break;
                    }
                }

                if (ob < 0) {
                    ob = 0;
                    sus = true;
                }

                auto &pattern = patterns[ob];
                uint32_t colorIndex = pattern[dx + dy * N];
                uint32_t color = colors[colorIndex];

                out.set(x, y,
                        {uint8_t((color >> 16) & 0xFF),
                         uint8_t((color >> 8) & 0xFF), uint8_t(color & 0xFF)});
            }
        }

        if (sus) {
            std::cerr << "get_output() called on contradicted wfc(overlap)"
                      << std::endl;
        }

        return out;
    };
};

#endif  // WFC_WFC_HPP_
