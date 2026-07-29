# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmimedatabase_malformed LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_qmimedatabase_malformed Test:
##
## Tests for QMimeDatabase parsing of untrusted,
## externally-installed MIME data.
##
## Every case runs its query in a child process, so QProcess is a hard
## requirement rather than something to skip at run time.
#####################################################################
if(NOT QT_FEATURE_process)
    return()
endif()

# The query itself runs here, in a process that is allowed to die: a crash means
# the parser is unsafe for the crafted mime.cache the test just wrote.
# The test looks the probe up next to its own binary, so it must land in the
# test's output directory rather than in the default executable one.
qt_internal_add_test_helper(mimecacheprobe
    OVERRIDE_OUTPUT_DIRECTORY
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    SOURCES
        mimecacheprobe.cpp
    LIBRARIES
        Qt::Core
)

qt_internal_add_test(tst_qmimedatabase_malformed
    SOURCES
        tst_qmimedatabase_malformed.cpp
    LIBRARIES
        Qt::Core
)

add_dependencies(tst_qmimedatabase_malformed mimecacheprobe)
