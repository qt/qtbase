# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qmyserver.pro.

#####################################################################
## qmyserver Binary:
#####################################################################

qt_internal_add_executable(qmyserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    NO_INSTALL # special case
    SOURCES
        ../myobject.h
        qmyserver.cpp
    LIBRARIES
        Qt::DBus
)
