# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qnetconmonitor.pro.

#####################################################################
## qnetconmonitor Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

qt_internal_add_manual_test(qnetconmonitor
    SOURCES
        tst_qnetconmonitor.cpp
    LIBRARIES
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetconmonitor.pro:<TRUE>:
# TEMPLATE = "app"
