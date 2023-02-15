# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qeventloop Test:
#####################################################################

qt_internal_add_test(tst_qeventloop
    SOURCES
        tst_qeventloop.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qeventloop CONDITION WIN32
    LIBRARIES
        user32
)

qt_internal_extend_target(tst_qeventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)
