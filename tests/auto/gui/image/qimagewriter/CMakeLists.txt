# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qimagewriter.pro.

#####################################################################
## tst_qimagewriter Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qimagewriter
    SOURCES
        tst_qimagewriter.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

