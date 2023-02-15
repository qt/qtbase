# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## almostplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(almostplugin
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        almostplugin.cpp almostplugin.h
    LIBRARIES
        Qt::Core
)

qt_autogen_tools_initial_setup(almostplugin)
