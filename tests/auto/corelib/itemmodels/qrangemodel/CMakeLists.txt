# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrangemodel Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qrangemodel LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qrangemodel
    SOURCES
        tst_qrangemodel.cpp
        data.h
    LIBRARIES
        Qt::Gui
)

if (
    # INTEGRITY and VxWorks don't come with all the concepts headers
    NOT INTEGRITY AND NOT VXWORKS AND
    # gcc 10.2.0 chokes in a standard library header
    (NOT CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10.2.0))
    if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_20")
        set_property(TARGET tst_qrangemodel PROPERTY CXX_STANDARD 20)
    endif()
endif()
