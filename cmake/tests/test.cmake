# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# FAKE moc-ing:
set(QT_MOCSCANNER /usr/bin/true)

# Fake mocscanner run.
# The files passed in after MOC will be reported to be in need of moc-ing,
# but will not be built.
# The files passed in after MOC_AND_BUILD will be reported to be in need
# of moc-ing and should also be built by the target.
function(fake_moc_results)
    cmake_parse_arguments(arg "" "" "MOC;MOC_AND_BUILD" ${ARGN})

    string(REPLACE ";" "\n" arg_MOC "${arg_MOC}")
    string(REPLACE ";" "\n" arg_MOC_AND_BUILD "${arg_MOC_AND_BUILD}")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/moc_files_included.txt" "${arg_MOC}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/moc_files_to_build.txt" "${arg_MOC_AND_BUILD}")
endfunction()

# Test whether a target has a file listed in its sources.
# Tests with the BUILD flag set will require this file to be built,
# while those without will require the file to not be built by
# the target.
function(test_source_file target file)
    cmake_parse_arguments(arg "BUILD" "" "" ${ARGN})

    get_target_property(sources "${target}" SOURCES)
    list(FIND sources "${file}" source_pos)
    assert(NOT source_pos STREQUAL "-1")

    get_source_file_property(prop "${file}" HEADER_FILE_ONLY)
    if (arg_BUILD)
        assert(NOT prop)
    else()
        assert(prop)
    endif()
endfunction()

# Test whether or not a target uses a header path
# The test passes when the path is in the list of include directories.
# Passing 'UNKNOWN' to this function reverses the test result.
function(test_include_directory target path)
    cmake_parse_arguments(arg "UNKNOWN" "" "" ${ARGN})
    get_target_property(includes "${target}" INCLUDE_DIRECTORIES)
    list(FIND includes "${path}" include_pos)
    if(arg_UNKNOWN)
        assert(include_pos STREQUAL "-1")
    else()
        assert(NOT include_pos STREQUAL "-1")
    endif()
endfunction()

# Add Core and Qt::Core libraries:
add_library(Core SHARED "${CMAKE_CURRENT_LIST_DIR}/empty.cpp")
add_library(Qt::Core ALIAS Core)
