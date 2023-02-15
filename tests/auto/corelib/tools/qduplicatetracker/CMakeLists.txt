# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qduplicatetracker Test:
#####################################################################

qt_internal_add_test(tst_qduplicatetracker
    SOURCES
        tst_qduplicatetracker.cpp
    LIBRARIES
        Qt::CorePrivate
)
