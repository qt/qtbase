# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from modal.pro.

#####################################################################
## modal Binary:
#####################################################################

qt_internal_add_executable(modal_helper  # special case
    SOURCES
        base.cpp base.h
        main.cpp
    OUTPUT_DIRECTORY # special case
        ${CMAKE_CURRENT_BINARY_DIR}/.. # special case
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################
