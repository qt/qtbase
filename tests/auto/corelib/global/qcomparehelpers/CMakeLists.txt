# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qcomparehelpers
    SOURCES
        tst_qcomparehelpers.h tst_qcomparehelpers.cpp
        wrappertypes.h
    LIBRARIES
        Qt::TestPrivate
)

# CMake recognizes CXX_STANDARD=23 only starting from version 3.20
# macOS has some issues with concepts, see QTBUG-117765
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.20" AND NOT MACOS)
    qt_internal_add_test(tst_qcomparehelpers_cpp23
        SOURCES
            tst_qcomparehelpers.h tst_qcomparehelpers.cpp
            wrappertypes.h
        DEFINES
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
