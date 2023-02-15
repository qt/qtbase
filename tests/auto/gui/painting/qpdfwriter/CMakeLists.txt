# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpdfwriter Test:
#####################################################################

qt_internal_add_test(tst_qpdfwriter
    SOURCES
        tst_qpdfwriter.cpp
    DEFINES
        QT_USE_USING_NAMESPACE
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
