cmake_minimum_required(VERSION 3.15.0)

project(QtBase
    VERSION 5.14.0
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

## Add some paths to check for cmake modules:
list(PREPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/extra-cmake-modules/find-modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/kwin"
)

## Find the build internals package.
list(PREPEND CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)
find_package(QtBuildInternals CMAKE_FIND_ROOT_PATH_BOTH)

qt_build_repo_begin(SKIP_CMAKE_MODULE_PATH_ADDITION)

## QtBase specific configure tests:
include(QtBaseConfigureTests)

## Build System tests:
include(QtBaseCMakeTesting)

## Targets for global features, etc.:
include(QtBaseGlobalTargets)

## Should this Qt be static or dynamically linked?
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
set(QT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

## Should this Qt be built with Werror?
option(WARNINGS_ARE_ERRORS "Build Qt with warnings as errors" ${FEATURE_developer_build})

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

qt_build_repo_end()
