# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdom.pro.

#####################################################################
## tst_qdom Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "doubleNamespaces.xml")
list(APPEND test_data "umlaut.xml")

qt_internal_add_test(tst_qdom
    SOURCES
        tst_qdom.cpp
    LIBRARIES
        Qt::Xml
    TESTDATA ${test_data}
)
