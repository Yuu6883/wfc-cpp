#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>

#include "external/rapidxml.hpp"
#include "image.hpp"
#include "overlapping_wfc.hpp"
#include "rapidxml_utils.hpp"
#include "time.h"
#include "utils.hpp"
#include "utils/array_3d.hpp"
#include "wfc.hpp"

using namespace rapidxml;
using namespace std;

/**
 * Get a random seed.
 * This function use random_device on linux, but use the C rand function for
 * other targets. This is because, for instance, windows don't implement
 * random_device non-deterministically.
 */
int get_random_seed() {
#ifdef __linux__
    return random_device()();
#else
    return rand();
#endif
}

Wave::Heuristic to_heuristic(const string &heuristic_string) {
#define CASE(enum) \
    if (heuristic_string == #enum) return Wave::Heuristic::enum

    CASE(Scanline);
    CASE(Entropy);
    CASE(MRV);

    throw runtime_error("Invalid Heuristic: " + heuristic_string);
#undef CASE
}

/**
 * Read the overlapping wfc problem from the xml node.
 */
void run_overlapping(xml_node<> *node) {
    string name = get_attribute(node, "name");

    auto size = get_attribute(node, "size", "48");
    uint32_t width = stoi(get_attribute(node, "width", size));
    uint32_t height = stoi(get_attribute(node, "height", size));

    uint32_t N = stoi(get_attribute(node, "N", "3"));

    bool periodic_output = get_attribute(node, "periodic", "False") == "True";
    bool periodic_input =
        get_attribute(node, "periodicInput", "True") == "True";

    bool ground = get_attribute(node, "ground", "False") == "True";
    uint32_t symmetry = stoi(get_attribute(node, "symmetry", "8"));
    uint32_t screenshots = stoi(get_attribute(node, "screenshots", "2"));
    string heuristic = get_attribute(node, "heuristic", "Entropy");

    cerr << "< " << name << endl;

    const string image_path = "samples/" + name + ".png";
    auto m = read_image(image_path);

    if (!m.has_value()) {
        throw runtime_error("Error while loading " + image_path);
    }

    auto &img = m.value();

    OverlappingWFC::Options options = {
        .periodic_input = periodic_input,
        .periodic_output = periodic_output,
        .i_W = img.MX,
        .i_H = img.MY,
        .o_W = width,
        .o_H = height,
        .symmetry = uint32_t((1 << symmetry) - 1),
        .pattern_size = N,
        .heuristic = to_heuristic(heuristic),
        .ground = ground,
    };

    OverlappingWFC wfc(options, img);

    for (uint32_t i = 0; i < screenshots; i++) {
        for (uint32_t test = 0; test < 10; test++) {
            int seed = get_random_seed();
            bool success = wfc.run(seed);

            cout << "> ";
            if (success) {
                cout << "DONE" << endl;
                write_image_png("results/" + name + to_string(seed) + ".png",
                                wfc.get_output());
                break;
            } else {
                cout << "CONTRADICTION" << endl;
            }
        }
    }
}

/**
 * Read a configuration file containing multiple wfc problems
 */
void read_config_file(const string &config_path) noexcept {
    ifstream config_file(config_path);
    vector<char> buffer((istreambuf_iterator<char>(config_file)),
                        istreambuf_iterator<char>());
    buffer.push_back('\0');
    auto document = new xml_document<>;
    document->parse<0>(&buffer[0]);

    xml_node<> *root_node = document->first_node("samples");
    string dir_path = get_dir(config_path) + "/" + "samples";
    for (xml_node<> *node = root_node->first_node("overlapping"); node;
         node = node->next_sibling("overlapping")) {
        run_overlapping(node);
    }

    delete document;
}

int main() {
// Initialize rand for non-linux targets
#ifndef __linux__
    srand(time(nullptr));
#endif

    using namespace chrono;

    time_point<system_clock> start, end;
    start = system_clock::now();

    read_config_file("samples.xml");

    end = system_clock::now();
    auto elapsed_ms = duration_cast<milliseconds>(end - start).count();
    cout << "All samples done in " << elapsed_ms << "ms.\n";
}
