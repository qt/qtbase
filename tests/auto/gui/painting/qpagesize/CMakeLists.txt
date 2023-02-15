# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpagesize Test:
#####################################################################

qt_internal_add_test(tst_qpagesize
    SOURCES
        tst_qpagesize.cpp
    DEFINES
        QT_USE_USING_NAMESPACE
    LIBRARIES
        Qt::Gui
)
