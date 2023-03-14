# Overview

This document gives an overview of the Qt 6 build system. For a hands-on guide on how
to build Qt 6, see https://doc.qt.io/qt-6/build-sources.html and
https://wiki.qt.io/Building_Qt_6_from_Git

# Contributing

See qtbase/cmake/CODESTYLE.md for the code style you should follow when contributing
to Qt's cmake files.

# CMake Versions

* You need CMake 3.16.0 or later for most platforms (due to new AUTOMOC json feature).
* You need CMake 3.17.0 to build Qt for iOS with the simulator_and_device feature.
* You need CMake 3.17.0 + Ninja to build Qt in debug_and_release mode on Windows / Linux.
* You need CMake 3.18.0 + Ninja to build Qt on macOS in debug_and_release mode when using
    frameworks.
* You need CMake 3.18.0 in user projects that use a static Qt together with QML
    (cmake_language EVAL is required for running the qmlimportscanner deferred finalizer)
* You need CMake 3.19.0 in user projects to use automatic deferred finalizers
    (automatic calling of qt_finalize_target)
* You need CMake 3.21.0 in user projects that create user libraries that link against a static Qt
    with a linker that is not capable to resolve circular dependencies between libraries
    (GNU ld, MinGW ld)

# Changes to Qt 5

The build system of Qt 5 was done on top of qmake. Qt 6 is built with CMake.

This offered an opportunity to revisit other areas of the build system, too:

* The Qt 5 build system allowed to build host tools during a cross-compilation run. Qt 6 requires
  you to build a Qt for your host machine first and then use the platform tools from that version. The
  decision to do this was reached independent of cmake: This does save resources on build machines
  as the host tools will only get built once.

* For now Qt still ships and builds bundled 3rd party code, due to time constraints on getting
  all the necessary pieces together in order to remove the bundled code (changes are necessary
  not only in the build system but in other parts of the SDK like the Qt Installer).

* There is less need for bootstrapping. Only moc and rcc (plus the lesser known tracegen and
  qfloat16-tables) are linking against the bootstrap Qt library. Everything else can link against
  the full QtCore. This does include qmake.
  qmake is supported as a build system for applications *using* Qt going forward and will
  not go away anytime soon.

# Building against homebrew on macOS

You may use brew to install dependencies needed to build QtBase.

  * Install homebrew:
    `/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`
  * Build Qt dependencies:  ``brew install pcre2 harfbuzz freetype``
  * Install cmake:  ``brew install cmake``
  * When running cmake in qtbase, pass ``-DFEATURE_pkg_config=ON`` together with
    ``-DCMAKE_PREFIX_PATH=/usr/local``, or ``-DCMAKE_PREFIX_PATH=/opt/homebrew`` if you have a Mac
    with Apple Silicon.

# Building

The basic way of building with cmake is as follows:

```
    cd {build directory}
    cmake -DCMAKE_INSTALL_PREFIX=/path/where/to/install {path to source directory}
    cmake --build .
    cmake --install .
```

The mapping of configure options to CMake arguments is described [here](configure-cmake-mapping.md).

You need one build directory per Qt module. The build directory can be a sub-directory inside the
module ``qtbase/build`` or an independent directory ``qtbase_build``. The installation prefix is
chosen when running cmake by passing ``-DCMAKE_INSTALL_PREFIX``. To build more than one Qt module,
make sure to pass the same install prefix.

``cmake --build`` and ``cmake --install`` are simple wrappers around the basic build tool that CMake
generated a build system for. It works with any supported build backend supported by cmake, but you
can also use the backend build tool directly, e.g. by running ``make``.

CMake has a ninja backend that works quite well and is noticeably faster (and more featureful) than
make, so you may want to use that:

```
    cd {build directory}
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/path/where/to/install {path to source directory}
    cmake --build .
    cmake --install .
```

You can look into the generated ``build.ninja`` file if you're curious and you can also build
targets directly, such as ``ninja lib/libQt6Core.so``.

Make sure to remove CMakeCache.txt if you forgot to set the CMAKE_INSTALL_PREFIX on the first
configuration, otherwise a second re-configuration will not pick up the new install prefix.

You can use ``cmake-gui {path to build directory}`` or ``ccmake {path to build directory}`` to
configure the values of individual cmake variables or Qt features. After changing a value, you need
to choose the *configure* step (usually several times:-/), followed by the *generate* step (to
generate makefiles/ninja files).

## Developer Build

When working on Qt itself, it can be tedious to wait for the install step. In that case you want to
use the developer build option, to get as many auto tests enabled and no longer be required to make
install:

```
    cd {build directory}
    cmake -GNinja -DFEATURE_developer_build=ON {path to source directory}
    cmake --build .
    # do NOT make install
```

## Specifying configure.json features on the command line

QMake defines most features in configure.json files, like -developer-build or -no-opengl.

In CMake land, we currently generate configure.cmake files from the configure.json files into
the source directory next to them using the helper script
``path_to_qtbase_source/util/cmake/configurejson2cmake.py``. They are checked into the repository.
If the feature in configure.json has the name "dlopen", you can specify whether to enable or disable that
feature in CMake with a -D flag on the CMake command line. So for example -DFEATURE_dlopen=ON or
-DFEATURE_sql_mysql=OFF. Remember to convert all '-' to '_' in the feature name.
At the moment, if you change a FEATURE flag's value, you have to remove the
CMakeCache.txt file and reconfigure with CMake. And even then you might stumble on some issues when
reusing an existing build, because of an automoc bug in upstream CMake.

## Building with CCache

You can pass ``-DQT_USE_CCACHE=ON`` to make the build system look for ``ccache`` in your ``PATH``
and prepend it to all C/C++/Objective-C compiler calls. At the moment this is only supported for the
Ninja and the Makefile generators.

## Cross Compiling

Compiling for a target architecture that's different than the host requires one build of Qt for the
host. This "host build" is needed because the process of building Qt involves the compilation of
intermediate code generator tools, that in turn are called to produce source code that needs to be
compiled into the final libraries. These tools are built using Qt itself and they need to run on the
machine you're building on, regardless of the architecture you are targeting.

Build Qt regularly for your host system and install it into a directory of your choice using the
``CMAKE_INSTALL_PREFIX`` variable. You are free to disable the build of tests and examples by
passing ``-DQT_BUILD_EXAMPLES=OFF`` and ``-DQT_BUILD_TESTS=OFF``.

With this installation of Qt in place, which contains all tools needed, we can proceed to create a
new build of Qt that is cross-compiled to the target architecture of choice. You may proceed by
setting up your environment. The CMake wiki has further information how to do that at

<https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling>

Yocto based device SDKs come with an environment setup script that needs to be sourced in your shell
and takes care of setting up environment variables and a cmake alias with a toolchain file, so that
you can call cmake as you always do.

In order to make sure that Qt picks up the code generator tools from the host build, you need to
pass an extra parameter to cmake:

```
    -DQT_HOST_PATH=/path/to/your/host_build
```

The specified path needs to point to a directory that contains an installed host build of Qt.

### Cross Compiling for Android

In order to cross-compile Qt to Android, you need a host build (see instructions above) and an
Android build. In addition, it is necessary to install the Android NDK.

The following CMake variables are required for an Android build:
  * `ANDROID_SDK_ROOT` must point to where the Android SDK is installed
  * `CMAKE_TOOLCHAIN_FILE` must point to the toolchain file that comes with the NDK
  * `QT_HOST_PATH` must point to a host installation of Qt

Call CMake with the following arguments:
`-DCMAKE_TOOLCHAIN_FILE=<path/to/ndk>/build/cmake/android.toolchain.cmake -DQT_HOST_PATH=/path/to/your/host/build -DANDROID_SDK_ROOT=<path/to/sdk> -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH`

The toolchain file is usually located below the NDK's root at "build/cmake/android.toolchain.cmake".
Instead of specifying the toolchain file you may specify `ANDROID_NDK_ROOT` instead.
This variable is exclusively used for auto-detecting the toolchain file.

In a recent SDK installation, the NDK is located in a subdirectory "ndk_bundle" below the SDK's root
directory. In that situation you may omit `ANDROID_NDK_ROOT` and `CMAKE_TOOLCHAIN_FILE`.

If you don't supply the configuration argument ``-DANDROID_ABI=...``, it will default to
``armeabi-v7a``. To target other architectures, use one of the following values:
  * arm64: ``-DANDROID_ABI=arm64-v8a``
  * x86: ``-DANDROID_ABI=x86``
  * x86_64: ``-DANDROID_ABI=x86_64``

By default we set the android API level to 28. Should you need to change this supply the following
configuration argument to the above CMake call: ``-DANDROID_PLATFORM=android-${API_LEVEL}``.

### Cross compiling for iOS

In order to cross-compile Qt to iOS, you need a host macOS build.

When running cmake in qtbase, pass
``-DCMAKE_SYSTEM_NAME=iOS -DQT_HOST_PATH=/path/to/your/host/build -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH``

If you don't supply the configuration argument ``-DQT_APPLE_SDK=...``, CMake will build a
multi-arch simulator_and_device iOS build.
To target another SDK / device type, use one of the following values:
  * iphonesimulator: ``-DQT_APPLE_SDK=iphonesimulator``
  * iphoneos: ``-DQT_APPLE_SDK=iphoneos``

Depending on what value you pass to ``-DQT_APPLE_SDK=`` a list of target architectures is chosen
by default:
  * iphonesimulator: ``x86_64``
  * iphoneos: ``arm64``
  * simulator_and_device: ``arm64;x86_64``

You can try choosing a different list of architectures by passing
``-DCMAKE_OSX_ARCHITECTURES=x86_64;i386``.
Note that if you choose different architectures compared to the default ones, the build might fail.
Only do it if you know what you are doing.

# Debugging CMake files

CMake allows specifying the ``--trace`` and ``--trace-expand`` options, which work like
``qmake -d -d``: As the cmake code is evaluated, the values of parameters and variables is shown.
This can be a lot of output, so you may want to redirect it to a file using the
``--trace-redirect=log.txt`` option.

# Porting Help

We have some python scripts to help with the conversion from qmake to cmake. These scripts can be
found in ``utils/cmake``.

## configurejson2cmake.py

This script converts all ``configure.json`` in the Qt repository to ``configure.cmake`` files for
use with CMake. We want to generate configure.cmake files for the foreseeable future, so if you need
to tweak the generated configure.cmake files, please tweak the generation script instead.

``configurejson2cmake.py`` is run like this: ``util/cmake/configurejson2cmake.py .`` in the
top-level source directory of a Qt repository.


## pro2cmake.py

``pro2cmake.py`` generates a skeleton CMakeLists.txt file from a .pro-file. You will need to polish
the resulting CMakeLists.txt file, but e.g. the list of files, etc. should be extracted for you.

``pro2cmake.py`` is run like this: ``path_to_qtbase_source/util/cmake/pro2cmake.py some.pro``.


## run_pro2cmake.py

`` A small helper script to run pro2cmake.py on all .pro-files in a directory. Very useful to e.g.
convert all the unit tests for a Qt module over to cmake;-)

``run_pro2cmake.py`` is run like this: ``path_to_qtbase_source/util/cmake/run_pro2cmake.py some_dir``.


## vcpkg support
The initial port used vcpkg to provide 3rd party packages that Qt requires.

At the moment the Qt CI does not use vcpkg anymore, and instead builds bundled 3rd party sources
if no relevant system package is found.

While the supporting code for building with vcpkg is still there, it is not tested at this time.


## How to convert certain constructs

| qmake                 | CMake                   |
| ------                | ------                  |
| ``qtHaveModule(foo)`` | ``if(TARGET Qt::foo)``  |
| ``qtConfig(foo)``     | ``if (QT_FEATURE_foo)`` |


# Convenience Scripts

A Qt installation's bin directory contains a number of convenience scripts.

## qt-cmake

This is a wrapper around the CMake executable which passes a Qt-internal `CMAKE_TOOLCHAIN_FILE`. Use
this to build projects against the installed Qt.

To use a custom toolchain file, use `-DQT_CHAINLOAD_TOOLCHAIN_FILE=<file path>`.

## qt-cmake-private

The same as `qt-cmake`, but in addition, sets the CMake generator to Ninja.

Example:

```
$ cd some/empty/directory
$ ~/Qt/6.0.0/bin/qt-cmake-private ~/source/of/qtdeclarative -DFEATURE_qml_network=OFF
$ cmake --build . && cmake --install .
```

## qt-configure-module

Call the configure script for a single Qt module, doing a CMake build.

Example:

```
$ cd some/empty/directory
$ ~/Qt/6.0.0/bin/qt-configure-module ~/source/of/qtdeclarative -no-feature-qml-network
$ cmake --build . && cmake --install .
```

## qt-cmake-standalone-test

Build a single standalone test outside the Qt build.

Example:

```
$ cd some/empty/directory
$ ~/Qt/6.0.0/bin/qt-cmake-standalone-test ~/source/of/qtbase/test/auto/corelib/io/qprocess
$ cmake --build .
```

## qt-cmake-create

Generates a simple CMakeLists.txt based on source files in specified project directory.

Example:

```
$ cd some/source/directory/
$ qt-cmake-create
$ qt-cmake -S . -B /build/directory
$ cmake --build /build/directory
```
