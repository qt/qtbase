# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
