# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qframe Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qframe
    LOWDPI
    SOURCES
        tst_qframe.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
