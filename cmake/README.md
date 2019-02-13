# Status

Initial port is on-going. Some modules of QtBase are ported, incl. some of the platform modules. Most are missing still.

Basic functionality is there (moc, uic, etc.), but documentation, translations, qdbusxml2cpp, etc. are missing.

NOTE: YOU WILL NEED CMAKE 3.14 or later (for example, master branch,
after a1b3d79f728f2c626d3aac2e68dbab524baa4634, for more details see
https://gitlab.kitware.com/cmake/cmake/merge_requests/2679 ).

# Intro

The CMake update offers an opportunity to revisit some topics that came up during the last few years.

* The Qt build system does not support building host tools during a cross-compilation run. You need to build a Qt for your host machine first and then use the platform tools from that version. The decision to do this was reached independent of cmake: This does save resources on build machines as the host tools will only get built once.

* 3rd-party dependencies are no longer built as part of Qt. zlib, libpng, etc. from src/3rdparty need
to be supplied from the outside to the build now. You may find apt-get/brew/etc. useful for this. Otherwise you may consider using vcpkg as in the next section. The decision to remove 3rd party dependencies from Qt repositories was reached independent of the decision to use cmake, we just use the opportunity to implement this decision.

* There is less need for bootstrapping. Only moc and rcc (plus the lesser known tracegen and qfloat16-tables) are linking against the bootstrap Qt library. Everything else can link against the full QtCore. This will include qmake, which is currently missing from a cmake build. This will change: Qmake is supported as a build system for applications *using* Qt going forward and will not go away anytime soon.

* For the time being we try to keep qmake working so that we do not interfere too much with ongoing development.


# Building against VCPKG

You may use vcpkg to install dependencies needed to build QtBase.

  * ```git clone -b qt https://github.com/tronical/vcpkg```
  * Run ```bootstrap-vcpkg.bat``` or ```bootstrap-vcpkg.sh```
  * Set the ``VCPKG_DEFAULT_TRIPLET`` environment variable to
    * Linux: ``x64-linux``
    * Windows: ``qt-x86-windows-static``
  * Build Qt dependencies:  ``vcpkg install zlib pcre2 double-conversion harfbuzz``
  * When running cmake in qtbase, pass ``-DCMAKE_PREFIX_PATH=/path/to/your/vcpkg/installed/$VCPKG_DEFAULT_TRIPLET`` or ``-DCMAKE_PREFIX_PATH=/path/to/your/vcpkg/installed/%VCPKG_DEFAULT_TRIPLET%`` on Windows.


# Building against homebrew on macOS

vcpkg doesn't support clang on macOS, see https://github.com/Microsoft/vcpkg/issues/4475 .

  * Install homebrew: ```/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"```
  * Build Qt dependencies:  ``brew install pcre2 harfbuzz``
  * Build cmake from HEAD (or you can build your own):  ``brew --HEAD install cmake``
  * When running cmake in qtbase, pass ``-DCMAKE_PREFIX_PATH=/usr/local``


# Building

The basic way of building with cmake is as follows:

```
    cd {build directory}
    cmake {path to source directory}
    cmake --build .
```

``cmake --build`` is just a simple wrapper around the basic build tool that CMake generated a build system for. It works with any supported build backend supported by cmake, but you can also use the backend build tool directly, e.g. by running ``make`` in this case.

CMake has a ninja backend that works quite well and is noticeably faster than make, so you may want to use that:

```
    cd {build directory}
    cmake -GNinja {path to source directory}
    cmake --build . # ... or ninja ;-)
```

You can look into the generated ``build.ninja`` file if you're curious and you can also build targets directory such as ``ninja lib/libQt5Core.so``.

When you're done with the build, you may want to install it, using ``ninja install`` or ``make install``. The installation prefix is chosen when running cmake though:

```
    cd {build directory}
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/path/where/to/install {path to source directory}
    ninja
    ninja install
```

You can use ``cmake-gui {path to build directory}`` or ``ccmake {path to build directory}`` to configure the values of individual cmake variables or Qt features. After changing a value, you need to choose the *configure* step (usually several times:-/), followed by the *generate* step (to generate makefiles/ninja files).

## Building with CCache

You can pass ``-DQT_USE_CCACHE=ON`` to make the build system look for ``ccache`` in your ``PATH`` and prepend it to all C/C++/Objective-C compiler calls. At the moment this is only supported for the Ninja and the Makefile generators.

## Cross Compiling

Compiling for a target architecture that's different than the host requires one build of Qt for the host. This "host build" is needed because the process of building Qt involves the compilation of intermediate code generator tools, that in turn are called to produce source code that needs to be compiled into the final libraries. These tools are built using Qt itself and they need to run on the machine you're building on, regardless of the architecure you are targeting.

Build Qt regularly for your host system and install it into a directory of your choice using the ``CMAKE_INSTALL_PREFIX`` variable.  You are free to disable the build of tests and examples by setting ``BUILD_EXAMPLES=OFF`` and ``BUILD_TESTING=OFF``.

With this installation of Qt in place, which contains all tools needed, we can proceed to create a new build of Qt that is cross-compiled to the target architecture of choice. You may proceed by setting up your environment. The CMake wiki has further information how to do that at

<https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling>

Yocto based device SDKs come with an environment setup script that needs to be sourced in your shell and takes care of setting up environment variables and a cmake alias with a toolchain file, so that you can call cmake as you always do.

In order to make sure that Qt picks up the code generator tools from the host build, you need to pass an extra parameter to cmake:

```
    -DHOST_QT_TOOLS_DIRECTORY=/path/to/your/host_build/bin
```

The specified path needs to point to a directory that contains all the binaries of the host build of Qt.

# Debugging CMake files

CMake allows specifying the ``--trace`` and ``--trace-expand`` options, which work like ``qmake -d -d``: As the cmake code is evaluated, the values of parameters and variables is shown. This can be a lot of output, so you may want to redirect it to a file.

# Porting Help

We have some python scripts to help with the conversion from qmake to cmake. These scripts can be found in ``utils/cmake``.

## configurejson2cmake.py

This script converts all ``configure.json`` in the Qt repository to ``configure.cmake`` files for use with CMake. We want to generate configure.cmake files for the foreseeable future, so if you need to tweak the generated configure.cmake files, please tweak the generation script instead.

``configurejson2cmake.py`` is run like this: ``util/cmake/configurejson2cmake.py .`` in the top-level source directory of a Qt repository.


## pro2cmake.py

``pro2cmake.py`` generates a skeleton CMakeLists.txt file from a .pro-file. You will need to polish the resulting CMakeLists.txt file, but e.g. the list of files, etc. should be extracted for you.

``pro2cmake.py`` is run like this: ``/path/to/pro2cmake.py some.pro``.


## run_pro2cmake.py

`` A small helper script to run pro2cmake.py on all .pro-files in a directory. Very useful to e.g. convert all the unit tests for a Qt module over to cmake;-)

``run_pro2cmake.py`` is run like this: ``/path/to/run_pro2cmake.py some_dir``.


## How to convert certain constructs

| qmake                 | CMake                   |
| ------                | ------                  |
| ``qtHaveModule(foo)`` | ``if(TARGET Qt::foo)``  |
| ``qtConfig(foo)``     | ``if (QT_FEATURE_foo)`` |

