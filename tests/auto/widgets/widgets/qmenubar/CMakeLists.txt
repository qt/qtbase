# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmenubar.pro.

#####################################################################
## tst_qmenubar Test:
#####################################################################

qt_internal_add_test(tst_qmenubar
    SOURCES
        tst_qmenubar.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmenubar CONDITION MACOS
    SOURCES
        tst_qmenubar_mac.mm
    LIBRARIES
        ${FWAppKit}
)
