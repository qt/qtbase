# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_qdesktopservices Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_qdesktopservices
    SOURCES
        tst_qdesktopservices.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::Gui
        Qt::Test
)
