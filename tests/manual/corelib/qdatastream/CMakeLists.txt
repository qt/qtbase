# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_manual_qdatastream LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_manual_qdatastream Test:
#####################################################################

qt_internal_add_manual_test(tst_manual_qdatastream
    SOURCES
        tst_manualqdatastream.cpp
    LIBRARIES
        Qt::Core
    ENABLE_AUTOGEN_TOOLS
        uic
)
