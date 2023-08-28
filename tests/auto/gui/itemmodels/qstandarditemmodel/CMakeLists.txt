# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstandarditemmodel Test:
#####################################################################

qt_internal_add_test(tst_qstandarditemmodel
    SOURCES
        tst_qstandarditemmodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
        Qt::TestPrivate
)
