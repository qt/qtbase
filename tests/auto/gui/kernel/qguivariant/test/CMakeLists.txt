# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguivariant Test:
#####################################################################

# Resources:
file(GLOB_RECURSE qguivariant_resource_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "data/*"
)

qt_internal_add_test(tst_qguivariant
    SOURCES
        tst_qguivariant.cpp
    INCLUDE_DIRECTORIES
        ../../../../other/qvariant_common
    LIBRARIES
        Qt::Gui
    TESTDATA ${qguivariant_resource_files}
    BUILTIN_TESTDATA
)
