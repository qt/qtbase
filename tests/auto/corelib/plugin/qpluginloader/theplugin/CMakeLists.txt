# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_cmake_library(theplugin
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        theplugin.cpp theplugin.h
    LIBRARIES
        Qt::Core
)
qt_autogen_tools_initial_setup(theplugin)

if (UNIX AND NOT APPLE)
    qt_internal_add_cmake_library(theoldplugin
        MODULE
        INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
        SOURCES
            theoldplugin.cpp theoldplugin.h
        LIBRARIES
            Qt::Core
    )
    qt_autogen_tools_initial_setup(theoldplugin)

    # Force unoptimized builds with debugging information so some "QTMETADATA !"
    # strings appear elsewhere in the binary.
    target_compile_options(theplugin PRIVATE -O0 -g3)
    target_compile_options(theoldplugin PRIVATE -O0 -g3)
endif()

