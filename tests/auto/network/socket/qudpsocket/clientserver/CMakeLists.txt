# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from clientserver.pro.

#####################################################################
## clientserver Binary:
#####################################################################

qt_internal_add_executable(clientserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Network
)
