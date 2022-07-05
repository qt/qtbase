# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_wasm Test:
#####################################################################

qt_internal_add_test(tst_localfileapi
    SOURCES
        tst_localfileapi.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
        Qt::Gui
        Qt::Widgets
)
