# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsplitter Test:
#####################################################################

# Collect test data
list(APPEND test_data "extradata.txt")
list(APPEND test_data "setSizes3.dat")

qt_internal_add_test(tst_qsplitter
    SOURCES
        tst_qsplitter.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
