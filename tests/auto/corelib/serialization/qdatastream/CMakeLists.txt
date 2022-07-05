# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdatastream.pro.

#####################################################################
## tst_qdatastream Test:
#####################################################################

# Collect test data
list(APPEND test_data "datastream.q42")
list(APPEND test_data "typedef.q5")

qt_internal_add_test(tst_qdatastream
    SOURCES
        tst_qdatastream.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
