# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qtimer
    SOURCES
        tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
)

if(QT_FEATURE_glib AND UNIX)
    qt_internal_add_test(tst_qtimer_no_glib
        SOURCES
            tst_qtimer.cpp
        DEFINES
            DISABLE_GLIB
        LIBRARIES
            Qt::CorePrivate
    )
endif()
