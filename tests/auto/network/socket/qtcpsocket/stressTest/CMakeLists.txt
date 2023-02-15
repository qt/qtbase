# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## stressTest Binary:
#####################################################################

qt_internal_add_executable(stressTest
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        Test.cpp Test.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)
