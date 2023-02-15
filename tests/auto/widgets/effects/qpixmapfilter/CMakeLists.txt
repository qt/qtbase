# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpixmapfilter Test:
#####################################################################

qt_internal_add_test(tst_qpixmapfilter
    SOURCES
        tst_qpixmapfilter.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
