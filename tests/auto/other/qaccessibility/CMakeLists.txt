# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_accessibility)
    return()
endif()

#####################################################################
## tst_qaccessibility Test:
#####################################################################

qt_internal_add_test(tst_qaccessibility
    SOURCES
        accessiblewidgets.h
        tst_qaccessibility.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qaccessibility CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    LIBRARIES
        m
)

qt_internal_extend_target(tst_qaccessibility CONDITION WIN32
    LIBRARIES
        ole32
        oleacc
        oleaut32
        uuid
)
