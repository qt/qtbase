# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from macgui.pro.

if(NOT APPLE)
    return()
endif()
if(NOT TARGET Qt::Widgets) # special case
    return()
endif()

#####################################################################
## tst_macgui Test:
#####################################################################

qt_internal_add_test(tst_macgui
    SOURCES
        guitest.cpp guitest.h
        tst_macgui.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:macgui.pro:<TRUE>:
# _REQUIREMENTS = "mac" "widgets"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_macgui CONDITION MACOS
    LIBRARIES
        ${FWApplicationServices}
)
