# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrawfont Test:
#####################################################################

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/testfont.ttf"
    "testfont_bold_italic.ttf"
    "testfont_os2_v1.ttf"
)

qt_internal_add_test(tst_qrawfont
    SOURCES
        tst_qrawfont.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)

