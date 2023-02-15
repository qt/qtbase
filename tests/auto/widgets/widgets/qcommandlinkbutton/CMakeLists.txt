# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcommandlinkbutton Test:
#####################################################################

qt_internal_add_test(tst_qcommandlinkbutton
    SOURCES
        tst_qcommandlinkbutton.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
