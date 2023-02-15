# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qobject Test:
#####################################################################

qt_internal_add_test(tst_qobject
    SOURCES
        tst_qobject.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::TestPrivate
)

## Scopes:
#####################################################################
add_subdirectory(signalbug)
add_dependencies(tst_qobject signalbug_helper)
