# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwidget Test:
#####################################################################

# Resources:
set(qwidget_resource_files
    "geometry-fullscreen.dat"
    "geometry-maximized.dat"
    "geometry.dat"
    "hellotr_la.qm"
)

qt_internal_add_test(tst_qwidget
    SOURCES
        tst_qwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${qwidget_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwidget CONDITION AIX
    COMPILE_OPTIONS
        -fpermissive
)

qt_internal_extend_target(tst_qwidget CONDITION WIN32
    LIBRARIES
        gdi32
        user32
)
