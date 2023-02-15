# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qglyphrun Test:
#####################################################################

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/test.ttf"
    "Ligatures.otf"
)

qt_internal_add_test(tst_qglyphrun
    SOURCES
        tst_qglyphrun.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)

