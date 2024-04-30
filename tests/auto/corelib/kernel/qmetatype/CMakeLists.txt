# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmetatype Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmetatype LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "./typeFlags.bin")

qt_internal_add_cmake_library(qmetatype_lib1
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qmetatype"
    SOURCES
        lib1.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)
qt_internal_add_cmake_library(qmetatype_lib2
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qmetatype"
    SOURCES
        lib2.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)
set_target_properties(qmetatype_lib1 PROPERTIES
    VERSION 1.0.0
    SOVERSION 0
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN ON
)
set_target_properties(qmetatype_lib2 PROPERTIES
    VERSION 1.0.0
    SOVERSION 0
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN ON
)

qt_internal_add_test(tst_qmetatype
    SOURCES
        tst_qmetatype.h tst_qmetatype.cpp tst_qmetatype2.cpp
        tst_qmetatype3.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
        Qt::Gui
        qmetatype_lib1
        qmetatype_lib2
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmetatype CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)
