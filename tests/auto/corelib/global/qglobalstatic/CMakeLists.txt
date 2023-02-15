# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qglobalstatic Test:
#####################################################################

qt_internal_add_test(tst_qglobalstatic
    EXCEPTIONS
    SOURCES
        tst_qglobalstatic.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)
