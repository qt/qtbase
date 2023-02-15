# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextlist Test:
#####################################################################

qt_internal_add_test(tst_qtextlist
    SOURCES
        ../qtextdocument/common.h
        tst_qtextlist.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
