# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstyleoption Test:
#####################################################################

qt_internal_add_test(tst_qstyleoption
    SOURCES
        tst_qstyleoption.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
