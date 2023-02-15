# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextmarkdownwriter Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/example.md")
list(APPEND test_data "data/blockquotes.md")

qt_internal_add_test(tst_qtextmarkdownwriter
    SOURCES
        tst_qtextmarkdownwriter.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
