# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qglyphrun.pro.

#####################################################################
## tst_qglyphrun Test:
#####################################################################

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/test.ttf"
)

qt_internal_add_test(tst_qglyphrun
    SOURCES
        tst_qglyphrun.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)

