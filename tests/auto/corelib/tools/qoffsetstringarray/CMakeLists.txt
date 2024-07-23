# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qoffsetstringarray Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qoffsetstringarray LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qoffsetstringarray
    SOURCES
        tst_qoffsetstringarray.cpp
    LIBRARIES
        Qt::CorePrivate
)
if (NOT VXWORKS)
    set_property(TARGET tst_qoffsetstringarray PROPERTY CXX_STANDARD 20)
endif()

if (CLANG)
    target_compile_options(tst_qoffsetstringarray
        PUBLIC -fbracket-depth=512)
elseif (GCC)
    # fconstexpr-depth= defaults to 512
endif()
