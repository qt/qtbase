# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpixmapcache Test:
#####################################################################

qt_internal_add_test(tst_qpixmapcache
    SOURCES
        tst_qpixmapcache.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
