# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstandardpaths Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qstandardpaths LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "tst_qstandardpaths.cpp")

qt_internal_add_test(tst_qstandardpaths
    SOURCES
        tst_qstandardpaths.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstandardpaths CONDITION boot2qt
    DEFINES
        SKIP_FINDEXECUTABLE
)
