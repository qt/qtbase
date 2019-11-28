# Status

Initial port is on-going. Some modules of QtBase are ported, incl. some of the platform modules.
Many libraries, tests and examples are still missing.

Basic functionality is there (moc, uic, etc.), but documentation, translations, etc. are missing.

NOTE: YOU NEED CMAKE 3.15 or later.

# Intro

The CMake update offers an opportunity to revisit some topics that came up during the last few
years.

* The Qt build system does not support building host tools during a cross-compilation run. You need
  to build a Qt for your host machine first and then use the platform tools from that version. The
  decision to do this was reached independent of cmake: This does save resources on build machines
  as the host tools will only get built once.

* 3rd-party dependencies are no longer built as part of Qt. zlib, libpng, etc. from src/3rdparty
  need to be supplied from the outside to the build now. You may find apt-get/brew/etc. useful for
  this. Otherwise you may consider using vcpkg as in the next section. The decision to remove 3rd
  party dependencies from Qt repositories was reached independent of the decision to use cmake, we
  just use the opportunity to implement this decision.

* There is less need for bootstrapping. Only moc and rcc (plus the lesser known tracegen and
  qfloat16-tables) are linking against the bootstrap Qt library. Everything else can link against
  the full QtCore. This will include qmake, which is currently missing from a cmake build. This will
  change: Qmake is supported as a build system for applications *using* Qt going forward and will
  not go away anytime soon.

* For the time being we try to keep qmake working so that we do not interfere too much with ongoing
  development.


# Building against VCPKG on Windows

You may use vcpkg to install dependencies needed to build QtBase.

  * ```git clone -b qt https://github.com/tronical/vcpkg```
  * Run ```bootstrap-vcpkg.bat``` or ```bootstrap-vcpkg.sh```
  * Set the ``VCPKG_DEFAULT_TRIPLET`` environment variable to ``qt-x64-windows-static`` or
    ``qt-x86-windows-static``
  * Set the ``VCPKG_ROOT`` environment variable to the path where you cloned vcpkg
  * Build Qt dependencies:  ``vcpkg install @qt-packages-windows.txt``
  * When running cmake in qtbase, support for vcpkg will be picked up automatically when the
    VCPKG_ROOT/VCPKG_DEFAULT_TRIPLET environment variable is set.

# Building against homebrew on macOS

You may use brew to install dependencies needed to build QtBase.

  * Install homebrew:
    ```/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"```
  * Build Qt dependencies:  ``brew install pcre2 harfbuzz freetype``
  * Install cmake:  ``brew install cmake``
  * When running cmake in qtbase, pass ``-DCMAKE_PREFIX_PATH=/usr/local``

# Building

The basic way of building with cmake is as follows:

```
    cd {build directory}
    cmake -DCMAKE_INSTALL_PREFIX=/path/where/to/install {path to source directory}
    cmake --build .
    cmake --install .
```

You need one build directory per Qt module. The build directory can be a sub-directory inside the
module ``qtbase/build`` or an independent directory ``qtbase_build``. The installation prefix is
chosen when running cmake by passing ``-DCMAKE_INSTALL_PREFIX``. To build more than one Qt module,
make sure to pass the same install prefix.

``cmake --build`` and ``cmake --install`` are simple wrappers around the basic build tool that CMake
generated a build system for. It works with any supported build backend supported by cmake, but you
can also use the backend build tool directly, e.g. by running ``make``.

CMake has a ninja backend that works quite well and is noticeably faster than make, so you may want
to use that:

```
    cd {build directory}
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/path/where/to/install {path to source directory}
    cmake --build .
    cmake --install .
```

You can look into the generated ``build.ninja`` file if you're curious and you can also build
targets directory such as ``ninja lib/libQt6Core.so``.

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
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/path/to/qtbase_build -DFEATURE_developer_build=ON {path to source directory}
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
-DFEATURE_sql_mysql=OFF. At the moment, if you change a FEATURE flag's value, you have to remove the
CMakeCache.txt file and reconfigure with CMake. And even then you might stumble on some issues when
reusing an existing build, because of an automoc bug in upstream CMake.

## Ninja reconfiguration bug

If you use the Ninja generator, there's a bug that after the first CMake configuration, if you run
ninja, it will do the reconfiguration step again. This is quite annoying and time consuming.

There is an open pull request that fixes the issue at
https://github.com/ninja-build/ninja/pull/1527. You can build your own Ninja executable until the
request is merged.

```
    cd {some directory}
    git clone https://github.com/ninja-build/ninja.git
    cd ninja && mkdir build && cd build
    git remote add fix git@github.com:mathstuf/ninja.git && git fetch --all
    git cherry-pick 29a565f18e01ce83ca14801f4684cd2acaf00d4c
    ../configure.py --bootstrap
    cp ninja /usr/local/bin/ninja
```

## Building with CCache

You can pass ``-DQT_USE_CCACHE=ON`` to make the build system look for ``ccache`` in your ``PATH``
and prepend it to all C/C++/Objective-C compiler calls. At the moment this is only supported for the
Ninja and the Makefile generators.

## Cross Compiling

Compiling for a target architecture that's different than the host requires one build of Qt for the
host. This "host build" is needed because the process of building Qt involves the compilation of
intermediate code generator tools, that in turn are called to produce source code that needs to be
compiled into the final libraries. These tools are built using Qt itself and they need to run on the
machine you're building on, regardless of the architecure you are targeting.

Build Qt regularly for your host system and install it into a directory of your choice using the
``CMAKE_INSTALL_PREFIX`` variable. You are free to disable the build of tests and examples by
passing ``-DBUILD_EXAMPLES=OFF`` and ``-DBUILD_TESTING=OFF``.

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
Android build. In addition, it is necessary to install the Android NDK as well as vcpkg. Vcpkg is
needed to supply third-party libraries that Qt requires but that are not part of the Android NDK.

Vcpkg for Android can be set up using the following steps:

  * ```git clone -b qt https://github.com/tronical/vcpkg```
  * Run ```bootstrap-vcpkg.bat``` or ```bootstrap-vcpkg.sh```
  * Set the ``VCPKG_DEFAULT_TRIPLET`` environment variable to one of the following values:
    * ``arm-android`` (armeabi-v7a)
    * ``arm64-android`` (arm64v8)
    * ``x86-android`` (x86)
    * ``x64-android`` (x86_64)
  * Set the ``VCPKG_ROOT`` environment variable to the path where you cloned vcpkg
  * Set the ``ANDROID_NDK_HOME`` environment variable to the path where you have installed the Android NDK.
  * Set the ``ANDROID_SDK_HOME`` environment variable to the path where you have installed the Android SDK.
  * Build Qt dependencies:  ``vcpkg install @qt-packages-android.txt``

When running cmake in qtbase, pass
``-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DQT_HOST_PATH=/path/to/your/host/build -DANDROID_SDK_ROOT=$ANDROID_SDK_HOME -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH``

If you don't supply the configuration argument ``-DANDROID_ABI=...``, it will default to
``armeabi-v7a``. To target other architectures, use one of the following values:
  * arm64: ``-DANDROID_ABI=arm64-v8``
  * x86: ``-DANDROID_ABI=x86``
  * x86_64: ``-DANDROID_ABI=x86_64``

By default we set the android API level to 21. Should you need to change this supply the following
configuration argument to the above CMake call: ``-DANDROID_NATIVE_API_LEVEL=${API_LEVEL}``

### Cross compiling for iOS

In order to cross-compile Qt to iOS, you need a host macOS build.
In addition, it is necessary to install a custom version of vcpkg. Vcpkg is
needed to supply third-party libraries that Qt requires, but that are not part of the iOS SDK.

Vcpkg for iOS can be set up using the following steps:

  * ```git clone -b qt https://github.com/alcroito/vcpkg```
  * Run ```bootstrap-vcpkg.sh```
  * Set the ``VCPKG_DEFAULT_TRIPLET`` environment variable to one of the following values:
    * ``x64-ios``   (simulator x86_64)
    * ``x86-ios``   (simulator i386)
    * ``arm64-ios`` (device arm64)
    * ``arm-ios``   (device armv7)
    * ``fat-ios``   (simulator_and_device x86_64 and arm64* - special considedrations)
  * Set the ``VCPKG_ROOT`` environment variable to the path where you cloned vcpkg
  * Build Qt dependencies:  ``vcpkg install @qt-packages-ios.txt``

When running cmake in qtbase, pass
``-DCMAKE_SYSTEM_NAME=iOS -DQT_HOST_PATH=/path/to/your/host/build -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH``

If you don't supply the configuration argument ``-DQT_UIKIT_SDK=...``, it will default to
``iphonesimulator``. To target another SDK / device type, use one of the following values:
  * iphonesimulator: ``-DQT_UIKIT_SDK=iphonesimulator``
  * iphoneos: ``-DQT_UIKIT_SDK=iphoneos``
  * simulator_and_device: ``-DQT_FORCE_SIMULATOR_AND_DEVICE=ON -DQT_UIKIT_SDK=``

Depending on what value you pass to ``-DQT_UIKIT_SDK=`` a list of target architectures is chosen
by default:
  * iphonesimulator: ``x86_64``
  * iphoneos: ``arm64``
  * simulator_and_device: ``arm64;x86_64``

You can try choosing a different list of architectures by passing
``-DCMAKE_OSX_ARCHITECTURES=x86_64;i386``.
Note that if you choose different architectures compared to the default ones, the build might fail.
Only do it if you know what you are doing.

####  simulator_and_device special considerations
To do a simulator_and_device build, a custom version of CMake is required in addition to the vcpkg
fork. The merge request can be found here:
https://gitlab.kitware.com/cmake/cmake/merge_requests/3617

After you build your own copy of CMake using this merge request, you need to use it for both
vcpkg and Qt.

Note that vcpkg prefers its own version of CMake when building packages.
Make sure to put your custom built CMake in PATH, and force vcpkg to use this CMake by running
``export VCPKG_FORCE_SYSTEM_BINARIES=1`` in your shell.

# Debugging CMake files

CMake allows specifying the ``--trace`` and ``--trace-expand`` options, which work like
``qmake -d -d``: As the cmake code is evaluated, the values of parameters and variables is shown.
This can be a lot of output, so you may want to redirect it to a file.

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


## How to convert certain constructs

| qmake                 | CMake                   |
| ------                | ------                  |
| ``qtHaveModule(foo)`` | ``if(TARGET Qt::foo)``  |
| ``qtConfig(foo)``     | ``if (QT_FEATURE_foo)`` |

