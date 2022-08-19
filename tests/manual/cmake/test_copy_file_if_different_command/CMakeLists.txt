# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# How to run the test:
# 1. Create the build directory, e.g. /home/user/build_test_copy_file_if_different
# 2. cd /home/user/build_test_copy_file_if_different
# 3. /path/to/Qt/bin/qt-cmake /path/to/Qt/Sources/qtbase/tests/manual/cmake/test_copy_file_if_different_command
# 4. cmake --build . --parallel
# 5. ctest
cmake_minimum_required(VERSION 3.16)

project(test_copy_file_if_different_command
    LANGUAGES CXX
)

if(NOT CMAKE_HOST_WIN32)
    message("Test only applicable for WIN32 platform. Nothing to do.")
    return()
endif()

if(CMAKE_CROSSCOMPILING)
    message("Test should only be run on host system. Crosscompiling is not supported.")
    return()
endif()

find_program(fsutil NAMES fsutil fsutil.exe)
if(NOT fsutil)
    message(WARNING "Unable to find 'fsutil' executable. Skipping the test")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

function(test_copy_file_command output_file test_data_base_name size)
    set(testdatasrc1 "${CMAKE_CURRENT_BINARY_DIR}/${test_data_base_name}1.bin")
    set(testdatasrc2 "${CMAKE_CURRENT_BINARY_DIR}/${test_data_base_name}2.bin")
    set(testdatadst "${CMAKE_CURRENT_BINARY_DIR}/${test_data_base_name}.bin")

    # Remove existing data first
    file(REMOVE "${testdatasrc1}" "${testdatasrc2}" "${testdatadst}")

    file(TO_NATIVE_PATH "${testdatasrc1}" native_testdatasrc)
    execute_process(COMMAND ${fsutil} file createNew "${native_testdatasrc}" ${size}
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Unable to allocate file ${native_testdatasrc}"
            " of size ${size} for test"
        )
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${testdatasrc1}" "${testdatasrc2}"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Unable to copy test data from ${testdatasrc1} to ${testdatasrc2}")
    endif()

    foreach(src_file_num RANGE 1 2)
        set(src_file "${testdatasrc${src_file_num}}")
        file(APPEND "${src_file}" "${src_file_num}")
        _qt_internal_copy_file_if_different_command(copy_command "${src_file}" "${testdatadst}")
        execute_process(COMMAND ${copy_command} RESULT_VARIABLE result)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Unable to execute the copy command ${copy_command}")
        endif()
    endforeach()

    set(${output_file} "${testdatadst}" PARENT_SCOPE)
endfunction()

test_copy_file_command(output_file1K testdata1K 1024)
test_copy_file_command(output_file2GB testdata2GB 2147483648)

add_executable(test_copy_if_different_command main.cpp)

enable_testing()
add_test(NAME "test_copy_if_different_command" COMMAND test_copy_if_different_command "${output_file1K}" "${output_file2GB}")
