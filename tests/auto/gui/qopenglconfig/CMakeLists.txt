# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qopenglconfig Test:
#####################################################################

# Collect test data
list(APPEND test_data "buglist.json")

qt_internal_add_test(tst_qopenglconfig
    SOURCES
        tst_qopenglconfig.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
