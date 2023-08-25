# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtimer Test:
#####################################################################

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qtimer
    SOURCES
        tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)

## Scopes:
#####################################################################
