# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcssparser Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*
)

qt_internal_add_test(tst_qcssparser
    SOURCES
        tst_qcssparser.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Xml
    TESTDATA ${test_data}
)
