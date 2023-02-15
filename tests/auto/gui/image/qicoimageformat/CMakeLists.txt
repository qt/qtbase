# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qicoimageformat Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qicoimageformat
    SOURCES
        tst_qicoimageformat.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

