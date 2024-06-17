# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2024 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

if (UNIX)
    qt_internal_add_cmake_library(tst_qlibrary_myarchlib
        SHARED
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
        SOURCES
            mylib.c
        LIBRARIES
            Qt::Core
    )

    set_target_properties(tst_qlibrary_myarchlib PROPERTIES
        OUTPUT_NAME myarchlib
        VERSION 1.0.0
        SOVERSION 1
    )

    if ("${TEST_architecture_arch}" STREQUAL "x86_64")
        qt_internal_add_cmake_library(tst_qlibrary_myarchlib_x86-64-v3
            DEFINES
                ARCH="x86-64-v3"
            SHARED
            OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../glibc-hwcaps/x86-64-v3"
            SOURCES
                mylib.c
            LIBRARIES
                Qt::Core
        )
        set_target_properties(tst_qlibrary_myarchlib_x86-64-v3 PROPERTIES
            OUTPUT_NAME myarchlib
            VERSION 1.0.0
            SOVERSION 1
        )
    endif()
endif()
