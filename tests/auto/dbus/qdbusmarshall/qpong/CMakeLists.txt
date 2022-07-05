# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qpong.pro.

#####################################################################
## qpong Binary:
#####################################################################

qt_internal_add_executable(qpong
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qpong.cpp
    LIBRARIES
        Qt::DBus
)
