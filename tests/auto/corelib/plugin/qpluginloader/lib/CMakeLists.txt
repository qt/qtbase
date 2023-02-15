# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpluginloaderlib Generic Library:
#####################################################################

qt_internal_add_cmake_library(tst_qpluginloaderlib
    SHARED
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        mylib.c
    LIBRARIES
        Qt::Core
)

if(WIN32)
    # CMake sets for Windows-GNU platforms the suffix "lib"
    set_property(TARGET tst_qpluginloaderlib PROPERTY PREFIX "")
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpluginloaderlib CONDITION MSVC
    DEFINES
        WIN32_MSVC
)

set_target_properties(tst_qpluginloaderlib PROPERTIES
    C_VISIBILITY_PRESET "default"
    CXX_VISIBILITY_PRESET "default"
)
