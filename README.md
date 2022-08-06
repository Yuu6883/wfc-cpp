# wfc-cpp

Only overlapping model is implemented.
The project is also refactored to **header-only**, so it's easier to include.

## Performance/Optimizations Considerations
Even though running the examples has very unstable runtime, profiling tools shows the percentage of each function is very stable. Most of the runtime is spent in the WFC::propagate function. One obvious place to improve was the locality of the sparse propagator lists: it was `vector<array<vector<unsigned>, 4>>` in the original C++ code and `int[][][]` in the original C# code. It takes 2 indexing getting to the actual sparse propagator list at index (direction, pattern) which is much discontinuous in the memory in the original implementations. So I implemented a more memory coherent way to iterate through the propagator: storing a flattened 2D table of the offset & length to the sparse lists, then store the sparse lists together in 1 continuous vector. This step is very important for the next part of optimization: memory packing. Most models were using enough memory to cause many cache misses, so if we adjust the byte sizes of the data containers in the wave and propagator, we can fit the smaller models in L2 or even L1, while the bigger ones might fit in L3. For example, the `compatible` 3D list can be reduced from int32_t to just uint8_t, since it's initialized to the number of compatible patterns at index (opposite_direction, pattern). All the models have less than 256 compatible pattern pairs in the samples so this change is safe. But the propagator lists can only be reduced to uint16_t, since there're few models that has more than 256 total patterns, but still less than 65536. This does have a significant impact on the run time since WFC::propagate is mostly load & store operations. Here's a comparison of all the data structures with size_t element vs uint8_t element (excluding models with 255+ patterns):
![Comparison](https://user-images.githubusercontent.com/38842891/183143794-b406bceb-8f62-4ec9-92b1-b8babd68b612.jpg)

Downside of this is pattern count is more limited. Potential fix would be compiling the WFC class with combinations of size templates, and picking the fitting one at runtime after determining the numeric limit of the data containers. Upside of this is that we squeeze a bit more performance (I'm guessing ~20% over the original implementation) while saving memory: `font` model would take 1GB while this packed version only takes 170MB (could be even lower if we don't preallocate the propagate stack and let it grow dynamically).

Another thing I've not yet to explored is using a dense propagator instead of sparse: keeping a (pattern, direction, pattern) size bit-3darray. The memory consumption of a dense propagator would be fixed while a sparse propagator depends on the sparsity and it uses more memory to store the table entries. [jdh's implementation](https://youtu.be/TO0Tx3w5abQ?t=661) uses a dense propagator and he's templating the pattern type and grid dimensions into the class which can definitely help the compiler optimize better, even though it won't be as flexible as the original.
 
## Requirements

C++-20 compatible compiler & CMake

## Building the examples

```
git clone https://github.com/Yuu6883/wfc-cpp && cd wfc-cpp/
mkdir build && cd build
cmake ../ && cmake --build .
```

Models are defined in `example/samples.xml`, and will put the results in `results` folder in the executable folder.

## Third-parties library

The files in `example/include/external/` come from:

-   RapidXML [https://github.com/dwd/rapidxml](https://github.com/dwd/rapidxml)
-   stb Library [https://github.com/nothings/stb](https://github.com/nothings/stb)

## Image samples

The image samples come from [https://github.com/mxgmn/WaveFunctionCollapse](https://github.com/mxgmn/WaveFunctionCollapse)

## Random notes

This repository is actually a detached forked from [fast-wfc](https://github.com/math-fehr/fast-wfc)

Renamed to **wfc-cpp** because original repository does not employ any real optimization on the algorithm, therefore the "fast" in the name makes 0 sense. The funny part is that it claims the "optimizations improves the execution time by an order of magnitude" (🚀?) when it's really just the C++ compiler doing a better job than C# compiler, not because the code is better. Maybe the C# version improved over time so I don't know how "slow" it was before.

Same WFC model can run and fail 10 times or just run once and succeed - no conclusion can be made from comparing the runtime of running the examples. An actual precise and scientific way to benchmark this would require both executable using the same random number generator and seed combined with A/B testing on varies model.

## Licence

Copyright (c) 2018-2019 Mathieu Fehr and Nathanaël Courant.
Copyright (c) 2022 Yuu

MIT License, see `LICENSE` for further details.
