# wfc-cpp

Fork of [fast-wfc](https://github.com/math-fehr/fast-wfc)

## WIP

Tile model is still being refactored

Renamed to **wfc-cpp** because original repository does not employ any real optimization on the algorithm, therefore the "fast" in the name makes 0 sense. The funny part is that it claims the "optimizations improves the execution time by an order of magnitude" (🚀🚀🚀) when it's really just the C++ compiler doing a better job than C# compiler. If you just run the examples and compare the time elapsed, the numbers are completely unless because the runtime heavily varies based on PRNG. Same WFC model can run and fail 10 times in the C++ version and run once and succeed in the C# version which takes far less time - no conclusion can be made from this kind of eyeballing. An actual precise and scientific way to benchmark this would require both executable using the same random number generator and seed combined with A/B testing on varies model.

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
