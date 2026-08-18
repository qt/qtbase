# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#
# tests/libfuzzer/CMakeLists.txt
#
# Builds and runs the hand-written libFuzzer targets below as part of the
# normal build/test run, alongside the qt_fuzzgen-generated tests in
# tests/fuzz.
#
# Every target here is a standalone CMake project (its own project() and
# find_package(Qt6 ...)) by design: that's what lets oss-fuzz, and the manual
# workflow in README, build each one independently against an already-built
# Qt. To keep that untouched, each target is configured and built here with
# ExternalProject_Add() against the Qt currently being built in this tree,
# instead of being folded in via add_subdirectory().
#
# Enabled by -DFEATURE_test_fuzz=ON during configure, and requires clang with
# -sanitize fuzzer-no-link (see README).

if(NOT (CLANG AND QT_FEATURE_sanitize_fuzzer_no_link))
    message(STATUS
        "tests/libfuzzer targets need clang with -sanitize fuzzer-no-link; skipping.")
    return()
endif()

if(NOT COMMAND ExternalProject_Add)
    include(ExternalProject)
endif()

_qt_internal_get_build_vars_for_external_projects(
    CMAKE_DIR_VAR _lf_qt_cmake_dir
    PREFIXES_VAR _lf_qt_prefixes
)

set(_lf_cmake_args "-DCMAKE_PREFIX_PATH:STRING=${_lf_qt_prefixes}")
if(QT_HOST_PATH OR CMAKE_CROSSCOMPILING)
    list(APPEND _lf_cmake_args
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${_lf_qt_cmake_dir}/qt.toolchain.cmake")
endif()

file(GLOB_RECURSE _lf_leaf_lists
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt"
)

foreach(_lf_list IN LISTS _lf_leaf_lists)
    get_filename_component(_lf_dir "${_lf_list}" DIRECTORY)
    get_filename_component(_lf_target "${_lf_dir}" NAME)
    string(REPLACE "/" "_" _lf_name "qtbase_${_lf_dir}")

    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${_lf_list}" _lf_fp_line
        REGEX "find_package\\(Qt6 REQUIRED COMPONENTS")
    string(REGEX REPLACE ".*COMPONENTS[ \t]+([A-Za-z0-9 ]+)\\).*" "\\1"
        _lf_components "${_lf_fp_line}")
    separate_arguments(_lf_components)

    set(_lf_deps "")
    set(_lf_skip FALSE)
    foreach(_lf_component IN LISTS _lf_components)
        if(TARGET "Qt::${_lf_component}")
            list(APPEND _lf_deps "Qt::${_lf_component}")
        else()
            message(STATUS
                "tests/libfuzzer/${_lf_dir}: Qt::${_lf_component} is not available; skipping ${_lf_name}.")
            set(_lf_skip TRUE)
        endif()
    endforeach()
    if(_lf_skip)
        continue()
    endif()

    set(_lf_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${_lf_name}-build")
    ExternalProject_Add(${_lf_name}
        SOURCE_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/${_lf_dir}"
        BINARY_DIR      "${_lf_binary_dir}"
        CMAKE_ARGS      ${_lf_cmake_args}
        INSTALL_COMMAND ""
        TEST_COMMAND    ""
        BUILD_ALWAYS    ON
        DEPENDS         ${_lf_deps}
    )

    add_test(
        NAME ${_lf_name}
        COMMAND "${_lf_binary_dir}/${_lf_target}" -max_total_time=30
        WORKING_DIRECTORY "${_lf_binary_dir}"
    )
    set_tests_properties(${_lf_name} PROPERTIES LABELS "fuzz;libfuzzer")
endforeach()

add_custom_target(tst_fuzz_libfuzzer_check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L libfuzzer
)
