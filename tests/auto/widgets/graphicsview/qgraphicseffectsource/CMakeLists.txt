# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicseffectsource Test:
#####################################################################

qt_internal_add_test(tst_qgraphicseffectsource
    SOURCES
        tst_qgraphicseffectsource.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
