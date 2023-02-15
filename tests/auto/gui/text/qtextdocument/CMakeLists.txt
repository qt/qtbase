# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextdocument Test:
#####################################################################

qt_internal_add_test(tst_qtextdocument
    SOURCES
        common.h
        tst_qtextdocument.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Xml
)
