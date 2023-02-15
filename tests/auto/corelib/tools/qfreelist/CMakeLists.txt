# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfreelist Test:
#####################################################################

qt_internal_add_test(tst_qfreelist
    SOURCES
        tst_qfreelist.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfreelist CONDITION NOT QT_FEATURE_private_tests
    SOURCES
        ../../../../../src/corelib/tools/qfreelist.cpp
)
