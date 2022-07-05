# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from stressTest.pro.

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

#### Keys ignored in scope 1:.:.:stressTest.pro:<TRUE>:
# MOC_DIR = ".moc/"
# TMP_DIR = ".tmp/"
