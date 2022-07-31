# wfc-cpp

Fork of [fast-wfc](https://github.com/math-fehr/fast-wfc)

# Requirements

C++-17 compatible compiler & CMake

# Install the library

```
git clone https://github.com/math-fehr/fast-wfc && cd fast-wfc/
mkdir build && cd build
cmake ../ && cmake --build .
```

# Run the models

```
cd example/
mkdir build && cd build
cmake ../ && cmake --build .
./examples
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
