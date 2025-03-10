Nova
====

Nova (working title) is game developed in C++ and Vulkan for Windows, Linux, Mac, iOS, and Android.

Model space
-----------

By default, Blender uses Z as the vertical axis and Y as the "forward" axis (into the screen). When modelling in Blender, orient the model facing the default camera (down the negative Y axis). On export, select Z as the forward axis and Y as the vertical axis. 

Building from source
--------------------

### Prerequisites

#### Linux

* cmake
* vcpkg
* Vulkan SDK

#### OS X and iOS

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

#### Android

Linux host with the same prerequisites as above, plus

* Android Studio
* Android NDK (can be installed through Android Studio)

Make sure environment variables ANDROID_HOME and ANDROID_NDK_HOME are set.

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

#### Android

On Android, the build output is an AAB bundle located under build/android/gradle_output/outputs/bundle, which can be installed using the [bundle tool](https://github.com/google/bundletool/releases).

Convert the bundle to APKs

```
    java -jar ~/Downloads/bundletool-all-1.18.1.jar build-apks --bundle=./app-debug.aab --output=output.apks --local-testing
```

Install the APKs

```
    java -jar ~/Downloads/bundletool-all-1.18.1.jar install-apks --apks=output.apks
```

#### iOS

To install on the device

```
    xcrun devicectl device install app --device <id> ./build/ios/debug/Debug-iphoneos/nova_exe.app
```

You can obtain the device ID with

```
    xcrun devicectl list devices
```

### Creating deployables

#### OS X

Build icon set for OSX

```
    brew install imagemagick

    cd ./platform/osx
    ./build_icon_set ./icon.png
```

After running the osx-release preset, create an .app bundle with

```
    cmake --install ./build/osx/release
```

#### Windows and Linux

Zip bundles are created by the release presets.
