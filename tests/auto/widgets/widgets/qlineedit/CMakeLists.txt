# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlineedit Test:
#####################################################################

qt_internal_add_test(tst_qlineedit
    SOURCES
        tst_qlineedit.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qlineedit CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
)
