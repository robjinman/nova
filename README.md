nova
====

Building from source
--------------------

### Prerequisites

#### linux

* cmake
* vcpkg
* Vulkan SDK

#### OS X

* XCode
* cmake
* vcpkg
* Vulkan SDK
* homebrew

#### Windows

* Visual Studio 17 2022
* cmake
* vcpkg
* Vulkan SDK

Make sure everything you need is on your PATH, including objdump.exe, which should be installed with Visual Studio. You can find the location of objdump by opening a Native Tools command prompt and typing `where objdump`.

### Build

To build, just run the relevant workflow from the project root.

To see the list of workflows

```
    cmake --workflow --list-presets
```

For example, to make a debug build on linux

```
    cmake --workflow --preset=linux-debug
```

You can also run the configure/build steps separately

```
    cmake --preset=linux-debug
    cmake --build --preset=linux-debug
```

#### Building icon set for OSX

```
    brew install imagemagick

    cd ./osx
    ./build_icon_set ./icon.png
```
