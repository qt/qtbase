# Generated from qfontdatabase.pro.

#####################################################################
## tst_qfontdatabase Test:
#####################################################################

# Collect test data
list(APPEND test_data "LED_REAL.TTF")

add_qt_test(tst_qfontdatabase
    SOURCES
        tst_qfontdatabase.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
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

add_qt_resource(tst_qfontdatabase "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

