# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from uic.pro.

#####################################################################
## tst_uic Test:
#####################################################################

qt_internal_add_test(tst_uic
    SOURCES
        tst_uic.cpp
)

# Add dependencies that are implicitly used inside the test
add_dependencies(tst_uic
    Qt::uic
)
