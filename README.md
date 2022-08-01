# wfc-cpp

Fork of [fast-wfc](https://github.com/math-fehr/fast-wfc)

## WIP

Tile model is still being refactored

Renamed to wfc-cpp because original repository does not employ any real optimization on the algorithm, therefore the "fast" in the name makes 0 sense. Original repo claims the "improving the execution time by an order of magnitude" without any evidence (🚀🚀🚀). If you just run the examples and compare the time elapsed, the numbers are completely unless, because WFC runtime heavily varies based on PRNG. Running all the overlapping models range from 15 to 30 seconds and the C++ version ranges from 10 to 20 seconds. A precised benchmark would require both executable using the same random number generator and seed combined with A/B testing.

The project is also refactored to **header-only**, so it's easier to include.

# Requirements

C++-20 compatible compiler & CMake

# Building the examples

```
git clone https://github.com/Yuu6883/wfc-cpp && cd wfc-cpp/
mkdir build && cd build
cmake ../ && cmake --build .
```

Models are defined in `example/samples.xml`, and will put the results in `results` folder in the executable folder.

# Third-parties library

The files in `example/include/external/` come from:

-   RapidXML [https://github.com/dwd/rapidxml](https://github.com/dwd/rapidxml)
-   stb Library [https://github.com/nothings/stb](https://github.com/nothings/stb)

# Image samples

The image samples come from [https://github.com/mxgmn/WaveFunctionCollapse](https://github.com/mxgmn/WaveFunctionCollapse)

# Licence

Copyright (c) 2018-2019 Mathieu Fehr and Nathanaël Courant.
Copyright (c) 2022 Yuu

MIT License, see `LICENSE` for further details.
