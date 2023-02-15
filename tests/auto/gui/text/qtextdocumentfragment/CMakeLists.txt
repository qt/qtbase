# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextdocumentfragment Test:
#####################################################################

qt_internal_add_test(tst_qtextdocumentfragment
    SOURCES
        tst_qtextdocumentfragment.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
