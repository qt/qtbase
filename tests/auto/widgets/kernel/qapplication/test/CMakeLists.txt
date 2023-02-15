# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## test Test:
#####################################################################

# Collect test data
list(APPEND test_data "../tmp/README")
list(APPEND test_data "../modal")

qt_internal_add_test(tst_qapplication
    SOURCES
        ../tst_qapplication.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qapplication CONDITION builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
