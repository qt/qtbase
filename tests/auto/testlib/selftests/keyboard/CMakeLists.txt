# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from keyboard.pro.

#####################################################################
## keyboard Binary:
#####################################################################

qt_internal_add_executable(keyboard
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_keyboard.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(keyboard)
# special case end
