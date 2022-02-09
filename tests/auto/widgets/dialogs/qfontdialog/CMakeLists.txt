# Generated from qfontdialog.pro.

#####################################################################
## tst_qfontdialog Test:
#####################################################################

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testfonts_resource_files
    "../../../shared/resources/test.ttf"
    "../../../shared/resources/testfont.ttf"
)

qt_internal_add_test(tst_qfontdialog
    SOURCES
        tst_qfontdialog.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${testfonts_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################
