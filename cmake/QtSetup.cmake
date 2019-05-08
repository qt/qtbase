## Set a default build type if none was specified
set(_default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(_default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${_default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo") # Set the possible values of build type for cmake-gui
endif()

# Appends a 'debug postfix' to library targets (not executables)
# e.g. lib/libQt5DBus_debug.5.12.0.dylib
if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
elseif(APPLE)
    set(CMAKE_DEBUG_POSTFIX "_debug")
endif()

## Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## Position independent code:
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Do not relink dependent libraries when no header has changed:
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# Default to hidden visibility for symbols:
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if(FEATURE_developer_build)
    set(QT_WILL_INSTALL OFF)
    # Handle non-prefix builds by setting the cmake install prefix to the project binary dir.
    if(PROJECT_NAME STREQUAL "QtBase")
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE PATH
            "Install path prefix, prepended onto install directories." FORCE)
    else()
        # No-op. While building another module, the CMAKE_INSTALL_PREFIX should be set on the
        # command line to point to the qtbase build dir.
    endif()
else()
    set(QT_WILL_INSTALL ON)
endif()

## Enable testing:
include(CTest)
enable_testing()

## Define some constants to check for certain platforms, etc:
include(QtPlatformSupport)

## add_qt_module and co.:
include(QtBuild)

## Qt Feature support:
include(QtFeature)

## Compiler optimization flags:
include(QtCompilerOptimization)

## Compiler flags:
include(QtCompilerFlags)

## Set up developer build:
qt_set_up_developer_build()

## Find host tools (if non native):
set(QT_HOST_PATH "" CACHE PATH "Installed Qt host directory path, used for cross compiling.")

if (CMAKE_CROSSCOMPILING AND NOT IS_DIRECTORY ${QT_HOST_PATH})
    message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
endif()

## Enable support for sanitizers:
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/extra-cmake-modules/modules/ECMEnableSanitizers.cmake)

option(QT_USE_CCACHE "Enable the use of ccache")
if(QT_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
