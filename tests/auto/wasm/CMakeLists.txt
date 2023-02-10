# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_wasm Test:
#####################################################################

qt_internal_add_test(tst_localfileapi
    SOURCES
        tst_localfileapi.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

qt_internal_add_test(tst_qwasmwindowstack
    SOURCES
        tst_qwasmwindowstack.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

qt_internal_add_test(tst_qwasmkeytranslator
    SOURCES
        tst_qwasmkeytranslator.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
)
