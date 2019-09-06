## Set a default build type if none was specified

# Set the QT_IS_BUILDING_QT variable so we can verify whether we are building
# Qt from source
set(QT_BUILDING_QT TRUE CACHE
    TYPE STRING "When this is present and set to true, it signals that we are building Qt from source.")

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
# e.g. lib/libQt6DBus_debug.5.12.0.dylib
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

# Detect non-prefix builds, either when the install prefix is set to the binary dir
# or when enabling developer builds and no prefix is specified.
# This detection only happens when building qtbase, and later is propagated via the generated
# QtBuildInternalsExtra.cmake file.
if (PROJECT_NAME STREQUAL "QtBase")
    if((CMAKE_INSTALL_PREFIX STREQUAL CMAKE_BINARY_DIR) OR
        (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND FEATURE_developer_build))

        set(__qt_will_install_value OFF)
        # Handle non-prefix builds by setting the CMake install prefix to point to qtbase's build
        # dir.
        # While building another repo (like qtsvg), the CMAKE_INSTALL_PREFIX or CMAKE_PREFIX_PATH
        # (either work) should be set on the command line to point to the qtbase build dir.
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE PATH
            "Install path prefix, prepended onto install directories." FORCE)
    else()
        set(__qt_will_install_value ON)
    endif()
    set(QT_WILL_INSTALL ${__qt_will_install_value} CACHE BOOL
        "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)
    unset(__qt_will_install_value)
endif()

# Specify the QT_SOURCE_TREE only when building qtbase. Needed by some tests when the tests are
# built as part of the project, and not standalone. For standalone tests, the value is set in
# QtBuildInternalsExtra.cmake.
if(PROJECT_NAME STREQUAL "QtBase")
    set(QT_SOURCE_TREE "${QtBase_SOURCE_DIR}" CACHE PATH
        "A path to the source tree of the previously configured QtBase project." FORCE)
endif()

if(FEATURE_developer_build)
    if(DEFINED QT_CMAKE_EXPORT_COMPILE_COMMANDS)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ${QT_CMAKE_EXPORT_COMPILE_COMMANDS})
    else()
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif()
    set(QT_BUILD_TESTING ON)
else()
    set(QT_BUILD_TESTING OFF)
endif()

## Set up testing
option(BUILD_TESTING "Build the testing tree." ${QT_BUILD_TESTING})
include(CTest)
enable_testing()

# Set up building of examples.
option(BUILD_EXAMPLES "Build Qt examples" ON)

## Define some constants to check for certain platforms, etc:
include(QtPlatformSupport)

## Android platform settings
if(ANDROID)
    include(QtPlatformAndroid)
endif()

## add_qt_module and co.:
include(QtBuild)

## Qt Feature support:
include(QtFeature)

## Compiler optimization flags:
include(QtCompilerOptimization)

## Compiler flags:
include(QtCompilerFlags)

## Set up non-prefix build:
qt_set_up_nonprefix_build()

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
