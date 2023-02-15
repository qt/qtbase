# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qprinter Test:
#####################################################################

qt_internal_add_test(tst_qprinter
    SOURCES
        tst_qprinter.cpp
    LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
)
