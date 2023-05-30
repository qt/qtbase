# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguitimer Test:
#####################################################################

qt_internal_add_test(tst_qguitimer
    SOURCES
        ../../../corelib/kernel/qtimer/tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

if(QT_FEATURE_glib AND UNIX)
    # only exists as a different dispatcher for XCB
    qt_internal_add_test(tst_qguitimer_no_glib
        SOURCES
            ../../../corelib/kernel/qtimer/tst_qtimer.cpp
        DEFINES
            DISABLE_GLIB
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
    )
endif()
