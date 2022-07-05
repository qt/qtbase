# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qprocess_and_guieventloop.pro.

#####################################################################
## tst_qprocess_and_guieventloop Test:
#####################################################################

qt_internal_add_test(tst_qprocess_and_guieventloop
    SOURCES
        tst_qprocess_and_guieventloop.cpp
    LIBRARIES
        Qt::Gui
)
add_subdirectory(write-read-write)
