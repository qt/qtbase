# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmenu Test:
#####################################################################

qt_internal_add_test(tst_qmenu
    SOURCES
        tst_qmenu.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmenu CONDITION MACOS
    SOURCES
        tst_qmenu_mac.mm
    LIBRARIES
        objc
)

qt_internal_extend_target(tst_qmenu CONDITION NOT MACOS
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
)
