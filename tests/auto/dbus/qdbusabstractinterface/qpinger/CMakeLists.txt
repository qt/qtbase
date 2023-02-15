# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qpinger Binary:
#####################################################################

qt_internal_add_executable(qpinger
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    NO_INSTALL
    SOURCES
        ../interface.cpp ../interface.h
        qpinger.cpp
    LIBRARIES
        Qt::DBus
)
