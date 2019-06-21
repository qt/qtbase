cmake_minimum_required(VERSION 3.15.0)

if(DEFINED ENV{VCPKG_ROOT})
    set(vcpkg_toolchain_file "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    get_filename_component(vcpkg_toolchain_file "${vcpkg_toolchain_file}" ABSOLUTE)

    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        get_filename_component(supplied_toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
        if(NOT supplied_toolchain_file STREQUAL vcpkg_toolchain_file)
            set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "")
        endif()
        unset(supplied_toolchain_file)
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg_toolchain_file}" CACHE STRING "" FORCE)
    message(STATUS "Using vcpkg from $ENV{VCPKG_ROOT}")
    if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
        message(STATUS "Using vcpkg triplet ${VCPKG_TARGET_TRIPLET}")
    endif()
    unset(vcpkg_toolchain_file)
endif()

project(QtBase
    VERSION 6.0.0
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

# As long as we use the mkspecs (for qplatformdefs.h), we need to always
# install it, especially when cross-compiling.
set(mkspecs_install_dir "${INSTALL_DATADIR}")
qt_path_join(mkspecs_install_dir ${QT_INSTALL_DIR} ${mkspecs_install_dir})

qt_copy_or_install(DIRECTORY "${PROJECT_SOURCE_DIR}/mkspecs"
                   DESTINATION ${mkspecs_install_dir})


qt_build_repo_end()

option(BUILD_EXAMPLES "Build Qt examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
