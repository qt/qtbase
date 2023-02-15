# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## generate Binary:
#####################################################################

qt_internal_add_executable(generate
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        /usr/include/freetype2
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)
