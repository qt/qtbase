# Generated from qfontmetrics.pro.

#####################################################################
## tst_qfontmetrics Test:
#####################################################################

qt_internal_add_test(tst_qfontmetrics
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

qt_internal_add_resource(tst_qfontmetrics "testfont"
    PREFIX
        "/fonts"
    FILES
        ${testfont_resource_files}
)

