# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfontdatabase Test:
#####################################################################

# Collect test data
list(APPEND test_data "LED_REAL.TTF")

qt_internal_add_test(tst_qfontdatabase
    SOURCES
        tst_qfontdatabase.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set_source_files_properties("../../../shared/resources/testfont_condensed.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_condensed.ttf"
)
set_source_files_properties("../../../shared/resources/testfont_italic.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_italic.ttf"
)
set_source_files_properties("../../../shared/resources/testfont_open.otf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_open.otf"
)
set(testdata_resource_files
    "../../../shared/resources/testfont.ttf"
    "../../../shared/resources/testfont_condensed.ttf"
    "../../../shared/resources/testfont_italic.ttf"
    "../../../shared/resources/testfont_open.otf"
    "LED_REAL.TTF"
)

qt_internal_add_resource(tst_qfontdatabase "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

