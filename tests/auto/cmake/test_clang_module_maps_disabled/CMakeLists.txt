# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_clang_module_maps_disabled)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Core REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

# Including a Qt header should not have picked up Qt's module map, and hence not
# built any Qt module. Verified for both executables and libraries, each with its
# own module cache so that one target can't mask a failure of the other.
function(add_module_maps_disabled_test target)
    set(modules_cache_path "${CMAKE_CURRENT_BINARY_DIR}/module-cache-${target}")

    target_link_libraries(${target} PRIVATE Qt6::Core)
    target_compile_options(${target} PRIVATE
        -fmodules
        -fcxx-modules
        "-fmodules-cache-path=${modules_cache_path}"
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-Dmodules_cache_path=${modules_cache_path}"
            -P "${CMAKE_CURRENT_LIST_DIR}/verify_no_qt_modules.cmake"
        VERBATIM
    )
endfunction()

qt6_add_executable(test_clang_module_maps_disabled main.cpp)
add_module_maps_disabled_test(test_clang_module_maps_disabled)

qt6_add_library(test_clang_module_maps_disabled_lib STATIC lib.cpp)
add_module_maps_disabled_test(test_clang_module_maps_disabled_lib)
