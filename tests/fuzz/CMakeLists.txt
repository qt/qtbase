# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#
# tests/fuzz/CMakeLists.txt
#
# Runs qt_fuzzgen at CMake configure time to generate fuzz test sources for
# all exported Qt classes in this submodule, then compiles them as part of
# Qt's normal build and registers them with CTest.
#
# Enabled by -DFEATURE_test_fuzz=ON during configure (requires qt_fuzzgen).

# QT_FUZZGEN_EXECUTABLE is set by configure.cmake
if(NOT QT_FUZZGEN_EXECUTABLE)
    message(STATUS "QT_FUZZGEN_EXECUTABLE not set; skipping fuzz test generation")
    return()
endif()

# Generated output lives in the binary dir so it never touches the source tree.
set(_FUZZ_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${_FUZZ_GEN_DIR}")

# Collect skiplists: base + OS-specific (SKIPLIST.Windows / SKIPLIST.Darwin / SKIPLIST.Linux)
set(_FUZZ_SKIPLIST_ARGS)
foreach(_sl SKIPLIST "SKIPLIST.${CMAKE_SYSTEM_NAME}")
    set(_sl_path "${CMAKE_CURRENT_SOURCE_DIR}/${_sl}")
    if(EXISTS "${_sl_path}")
        list(APPEND _FUZZ_SKIPLIST_ARGS --skiplist "${_sl_path}")
    endif()
endforeach()

# Run qt_fuzzgen at configure time
execute_process(
    COMMAND "${QT_FUZZGEN_EXECUTABLE}"
        --submodule "${CMAKE_SOURCE_DIR}"
        --out "${_FUZZ_GEN_DIR}"
        --embedded
        ${_FUZZ_SKIPLIST_ARGS}
    RESULT_VARIABLE _FUZZ_RESULT
    OUTPUT_VARIABLE _FUZZ_OUT_LOG
    ERROR_VARIABLE  _FUZZ_ERR_LOG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)

if(NOT _FUZZ_RESULT EQUAL 0)
    message(WARNING "qt_fuzzgen failed (exit ${_FUZZ_RESULT}):\n${_FUZZ_ERR_LOG}")
    return()
endif()

message(STATUS "${_FUZZ_OUT_LOG}")

if(NOT EXISTS "${_FUZZ_GEN_DIR}/CMakeLists.txt")
    message(STATUS "qt_fuzzgen produced no fuzz tests.")
    return()
endif()

# Include the generated fuzz tree into this build.
# The generated top-level CMakeLists.txt is in embedded mode: no project(),
# no find_package() — it uses Qt's targets that are already defined here.
add_subdirectory("${_FUZZ_GEN_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/fuzz_build")
