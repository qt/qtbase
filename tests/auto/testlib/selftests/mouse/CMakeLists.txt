# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## mouse Binary:
#####################################################################

qt_internal_add_executable(mouse
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_mouse.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(mouse)
