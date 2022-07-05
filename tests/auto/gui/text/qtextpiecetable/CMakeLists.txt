# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtextpiecetable.pro.

if(WIN32)
    return()
endif()
if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qtextpiecetable Test:
#####################################################################

qt_internal_add_test(tst_qtextpiecetable
    SOURCES
        ../qtextdocument/common.h
        tst_qtextpiecetable.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:qtextpiecetable.pro:<TRUE>:
# _REQUIREMENTS = "!win32" "qtConfig(private_tests)"
