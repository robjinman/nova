Vulkan Test Lab
===============

Building from source
--------------------

### Linux

Install the development dependencies. On Ubuntu 20, run

```
        sudo apt install \
          build-essential \
          libvulkan-dev \
          vulkan-validationLayers-dev
```

To build third-party libraries, from the project root, run

```
        mkdir -p vendor/build/linux
        cd vendor/build/linux
        cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../..
        make -j4
```

To build the app, from project root, run

```
        mkdir -p build/linux
        cd build/linux
        cmake -G "Unix Makefiles" ../..
        make -j4
```

To-Do
-----

* Swap chain recreation on window resize and minimization

