# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcomparehelpers LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qcomparehelpers
    SOURCES
        tst_qcomparehelpers.h tst_qcomparehelpers.cpp tst_qcomparehelpers1.cpp
        wrappertypes.h
    NO_BATCH # QTBUG-121815
    DEFINES
        QTEST_THROW_ON_FAIL
        QTEST_THROW_ON_SKIP
    LIBRARIES
        Qt::TestPrivate
)

# CMake recognizes CXX_STANDARD=23 only starting from version 3.20
# macOS has some issues with concepts, see QTBUG-117765
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.20" AND NOT MACOS AND NOT VXWORKS AND NOT (LINUX AND "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64"))
    qt_internal_add_test(tst_qcomparehelpers_cpp23
        SOURCES
            tst_qcomparehelpers.h tst_qcomparehelpers.cpp tst_qcomparehelpers1.cpp
            wrappertypes.h
        NO_BATCH # QTBUG-121815
        DEFINES
            QTEST_THROW_ON_FAIL
            QTEST_THROW_ON_SKIP
            tst_QCompareHelpers=tst_QCompareHelpersCpp23
        LIBRARIES
            Qt::TestPrivate
    )

    # Try to build this test in C++23 mode to test std::float16_t support.
    # Use CXX_STANDARD_REQUIRED OFF, so that we just fall back to C++17 if the
    # compiler does not support C++23.
    set_target_properties(tst_qcomparehelpers_cpp23
        PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED OFF
    )
endif()
