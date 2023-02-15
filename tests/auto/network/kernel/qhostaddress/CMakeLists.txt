# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhostaddress Test:
#####################################################################

qt_internal_add_test(tst_qhostaddress
    SOURCES
        tst_qhostaddress.cpp
    LIBRARIES
        Qt::NetworkPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qhostaddress CONDITION WIN32
    LIBRARIES
        ws2_32
)
