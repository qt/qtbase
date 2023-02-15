# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpointer Test:
#####################################################################

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qpointer
    SOURCES
        tst_qpointer.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpointer CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
