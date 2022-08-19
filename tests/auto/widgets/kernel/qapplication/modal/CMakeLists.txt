# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
