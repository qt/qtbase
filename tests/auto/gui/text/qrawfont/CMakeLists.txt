# Generated from qrawfont.pro.

#####################################################################
## tst_qrawfont Test:
#####################################################################

add_qt_test(tst_qrawfont
    SOURCES
        tst_qrawfont.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/testfont.ttf"
    "testfont_bold_italic.ttf"
    "testfont_os2_v1.ttf"
)

add_qt_resource(tst_qrawfont "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

