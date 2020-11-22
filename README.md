Vulkan Test Lab
===============

Building from source
--------------------

### Linux

Install the development dependencies

```
        sudo apt-get install build-essential libvulkan-dev
```

To build third-party libraries, from the project root, run

```
        mkdir -p vendor/build/linux
        cd vendor/build/linux
        cmake -G "Unix Makefiles" ../..
        make -j4
```

To build the app, from project root, run

```
        mkdir -p build/linux
        cd build/linux
        cmake -G "Unix Makefiles" ../..
        make -j4
```
