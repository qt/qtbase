## Set a default build type if none was specified

# Set the QT_IS_BUILDING_QT variable so we can verify whether we are building
# Qt from source
set(QT_BUILDING_QT TRUE CACHE
    TYPE STRING "When this is present and set to true, it signals that we are building Qt from source.")

# Pre-calculate the developer_build feature if it's set by the user via INPUT_developer_build
if(NOT FEATURE_developer_build AND INPUT_developer_build
        AND NOT "${INPUT_developer_build}" STREQUAL "undefined")
    set(FEATURE_developer_build ON)
endif()

set(_default_build_type "Release")
if(FEATURE_developer_build)
    set(_default_build_type "Debug")
endif()

# Reset content of extra build internal vars for each inclusion of QtSetup.
unset(QT_EXTRA_BUILD_INTERNALS_VARS)

# Save the global property in a variable to make it available to feature conditions.
get_property(QT_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${_default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE
      PROPERTY STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo") # Set the possible values for cmake-gui.
elseif(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Building for multiple configurations: ${CMAKE_CONFIGURATION_TYPES}.")
    message(STATUS "Main configuration is: ${QT_MULTI_CONFIG_FIRST_CONFIG}.")
    if(CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE)
        message(STATUS
            "Default build configuration set to '${CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE}'.")
    endif()
endif()

# Appends a 'debug postfix' to library targets (not executables)
# e.g. lib/libQt6DBus_debug.5.12.0.dylib
if(WIN32)
    if(MINGW)
        # On MinGW we don't have "d" suffix for debug libraries like on Linux,
        # unless we're building debug and release libraries in one go.
        if(QT_GENERATOR_IS_MULTI_CONFIG)
            set(CMAKE_DEBUG_POSTFIX "d")
        endif()
    else()
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
elseif(APPLE)
    set(CMAKE_DEBUG_POSTFIX "_debug")
    set(CMAKE_FRAMEWORK_MULTI_CONFIG_POSTFIX_DEBUG "_debug")
endif()

## Position independent code:
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Do not relink dependent libraries when no header has changed:
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# Detect non-prefix builds: either when the qtbase install prefix is set to the binary dir
# or when a developer build is explicitly enabled and no install prefix is specified.
# This detection only happens when building qtbase, and later is propagated via the generated
# QtBuildInternalsExtra.cmake file.
if (PROJECT_NAME STREQUAL "QtBase" AND NOT QT_BUILD_STANDALONE_TESTS)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND FEATURE_developer_build)
        # Handle non-prefix builds by setting the CMake install prefix to point to qtbase's build
        # dir.
        # While building another repo (like qtsvg) the CMAKE_PREFIX_PATH
        # should be set on the command line to point to the qtbase build dir.
        set(CMAKE_INSTALL_PREFIX ${QtBase_BINARY_DIR} CACHE PATH
            "Install path prefix, prepended onto install directories." FORCE)
    endif()
    if(CMAKE_CROSSCOMPILING)
        set(__qt_prefix "${CMAKE_STAGING_PREFIX}")
    else()
        set(__qt_prefix "")
    endif()
    if(__qt_prefix STREQUAL "")
        set(__qt_prefix "${CMAKE_INSTALL_PREFIX}")
    endif()
    if(__qt_prefix STREQUAL QtBase_BINARY_DIR)
        set(__qt_will_install_value OFF)
    else()
        set(__qt_will_install_value ON)
    endif()
    set(QT_WILL_INSTALL ${__qt_will_install_value} CACHE BOOL
        "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)
    unset(__qt_prefix)
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
    set(_qt_build_tests_default ON)
    set(__build_benchmarks ON)

    # Tests are not built by default with qmake for iOS and friends, and thus the overall build
    # tends to fail. Disable them by default when targeting uikit.
    if(UIKIT OR ANDROID)
        set(_qt_build_tests_default OFF)
    endif()

    # Disable benchmarks for single configuration generators which do not build
    # with release configuration.
    if (CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL Release)
        set(__build_benchmarks OFF)
    endif()
else()
    set(_qt_build_tests_default OFF)
    set(__build_benchmarks OFF)
endif()

# Build Benchmarks
option(QT_BUILD_BENCHMARKS "Build Qt Benchmarks" ${__build_benchmarks})
if(QT_BUILD_BENCHMARKS)
    set(_qt_build_tests_default ON)
endif()

## Set up testing
option(QT_BUILD_TESTS "Build the testing tree." ${_qt_build_tests_default})
unset(_qt_build_tests_default)
option(QT_BUILD_TESTS_BY_DEFAULT "Should tests be built as part of the default 'all' target." ON)
if(QT_BUILD_STANDALONE_TESTS)
    # BuildInternals might have set it to OFF on initial configuration. So force it to ON when
    # building standalone tests.
    set(QT_BUILD_TESTS ON CACHE BOOL "Build the testing tree." FORCE)

    # Also force the tests to be built as part of the default build target.
    set(QT_BUILD_TESTS_BY_DEFAULT ON CACHE BOOL
        "Should tests be built as part of the default 'all' target." FORCE)
endif()
set(BUILD_TESTING ${QT_BUILD_TESTS} CACHE INTERNAL "")

# When cross-building, we don't build tools by default. Sometimes this also covers Qt apps as well.
# Like in qttools/assistant/assistant.pro, load(qt_app), which is guarded by a qtNomakeTools() call.

set(_qt_build_tools_by_default_default ON)
if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
    set(_qt_build_tools_by_default_default OFF)
endif()
option(QT_BUILD_TOOLS_BY_DEFAULT "Should tools be built as part of the default 'all' target."
       "${_qt_build_tools_by_default_default}")
unset(_qt_build_tools_by_default_default)

include(CTest)
enable_testing()

option(QT_BUILD_EXAMPLES "Build Qt examples" OFF)
option(QT_BUILD_EXAMPLES_BY_DEFAULT "Should examples be built as part of the default 'all' target." ON)

option(QT_BUILD_MANUAL_TESTS "Build Qt manual tests" OFF)
option(QT_BUILD_MINIMAL_STATIC_TESTS "Build minimal subset of tests for static Qt builds" OFF)

## Find host tools (if non native):
set(QT_HOST_PATH "" CACHE PATH "Installed Qt host directory path, used for cross compiling.")

if (CMAKE_CROSSCOMPILING)
    if(NOT IS_DIRECTORY "${QT_HOST_PATH}")
        message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
    endif()
endif()

if(NOT "${QT_HOST_PATH}" STREQUAL "")
    find_package(${INSTALL_CMAKE_NAMESPACE}HostInfo
                 CONFIG
                 REQUIRED
                 PATHS "${QT_HOST_PATH}"
                       "${QT_HOST_PATH_CMAKE_DIR}"
                 NO_CMAKE_FIND_ROOT_PATH
                 NO_DEFAULT_PATH)
endif()

## Android platform settings
if(ANDROID)
    include(QtPlatformAndroid)
endif()

## qt_add_module and co.:
include(QtBuild)

## Qt Feature support:
include(QtBuildInformation)
include(QtFeature)

## Compiler optimization flags:
include(QtCompilerOptimization)

## Compiler flags:
include(QtCompilerFlags)

## Set up non-prefix build:
qt_set_up_nonprefix_build()

qt_set_language_standards()

## Enable support for sanitizers:
qt_internal_set_up_sanitizer_features()
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/extra-cmake-modules/modules/ECMEnableSanitizers.cmake)

option(QT_USE_CCACHE "Enable the use of ccache")
if(QT_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(WARNING "Ccache use was requested, but the program was not found.")
    endif()
endif()

# We need to clean up QT_FEATURE_*, but only once per configuration cycle
get_property(qt_feature_clean GLOBAL PROPERTY _qt_feature_clean)
if(NOT qt_feature_clean)
    message(STATUS "Check for feature set changes")
    set_property(GLOBAL PROPERTY _qt_feature_clean TRUE)
    foreach(feature ${QT_KNOWN_FEATURES})
        if(DEFINED "FEATURE_${feature}" AND
            NOT "${QT_FEATURE_${feature}}" STREQUAL "${FEATURE_${feature}}")
            message("    '${feature}' is changed from ${QT_FEATURE_${feature}} \
to ${FEATURE_${feature}}")
            set(dirty_build TRUE)
        endif()
        unset("QT_FEATURE_${feature}" CACHE)
    endforeach()

    set(QT_KNOWN_FEATURES "" CACHE INTERNAL "" FORCE)

    if(dirty_build)
        set_property(GLOBAL PROPERTY _qt_dirty_build TRUE)
        message(WARNING "Re-configuring in existing build folder. \
Some features will be re-evaluated automatically.")
    endif()
endif()
