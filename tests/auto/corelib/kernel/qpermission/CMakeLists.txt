# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_FEATURE_permissions)
    return()
endif()

qt_internal_add_test(tst_qpermission
    SOURCES
        tst_qpermission.cpp
    LIBRARIES
        Qt::Core
)
