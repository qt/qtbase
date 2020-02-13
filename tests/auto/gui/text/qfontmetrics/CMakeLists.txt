# Generated from qfontmetrics.pro.

#####################################################################
## tst_qfontmetrics Test:
#####################################################################

add_qt_test(tst_qfontmetrics
    SOURCES
        tst_qfontmetrics.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testfont_resource_files
    "../../../shared/resources/testfont.ttf"
    "ucs4font.ttf"
)

add_qt_resource(tst_qfontmetrics "testfont"
    PREFIX
        "/fonts"
    FILES
        ${testfont_resource_files}
)

