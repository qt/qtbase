# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhostinfo Test:
#####################################################################

qt_internal_add_test(tst_qhostinfo
    SOURCES
        tst_qhostinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qhostinfo CONDITION WIN32
    LIBRARIES
        ws2_32
)
