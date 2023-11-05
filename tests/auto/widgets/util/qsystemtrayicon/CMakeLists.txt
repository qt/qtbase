# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsystemtrayicon Test:
#####################################################################

set(resources_resource_files
    "icons/icon.png"
)

qt_internal_add_test(tst_qsystemtrayicon
    SOURCES
        tst_qsystemtrayicon.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${resources_resource_files}
    BUILTIN_TESTDATA
)
