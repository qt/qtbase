cmake_minimum_required(VERSION 3.14.0)

project(QtBase
    VERSION 5.12.0
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

## Add some paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/extra-cmake-modules/find-modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/kwin"
)

## Qt specific setup common for all modules:
include(QtSetup)

## Enable feature summary at the end of the configure run:
include(FeatureSummary)

## QtBase specific configure tests:
include(QtBaseConfigureTests)

## Build System tests:
include(QtBaseCMakeTesting)

## Targets for global features, etc.:
include(QtBaseGlobalTargets)

## Should this Qt be static or dynamically linked?
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
set(QT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

## Decide whether tools will be built.
qt_check_if_tools_will_be_built()

## Visit all the directories:
add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

if (QT_WILL_BUILD_TOOLS)
    add_subdirectory(qmake)
endif()

option(BUILD_EXAMPLES "Build Qt examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

## Delayed actions on some of the Qt targets:
include(QtPostProcess)

## Print a feature summary:
feature_summary(WHAT PACKAGES_FOUND
                     REQUIRED_PACKAGES_NOT_FOUND
                     RECOMMENDED_PACKAGES_NOT_FOUND
                     OPTIONAL_PACKAGES_NOT_FOUND
                     RUNTIME_PACKAGES_NOT_FOUND
                     FATAL_ON_MISSING_REQUIRED_PACKAGES)
