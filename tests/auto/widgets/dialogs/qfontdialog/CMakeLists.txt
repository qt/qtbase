# Generated from qfontdialog.pro.

#####################################################################
## tst_qfontdialog Test:
#####################################################################

add_qt_test(tst_qfontdialog
    SOURCES
        tst_qfontdialog.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES alias "test.ttf")
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES alias "testfont.ttf")
add_qt_resource(tst_qfontdialog "testfonts" PREFIX "/" FILES
    ../../../shared/resources/test.ttf
    ../../../shared/resources/testfont.ttf)


#### Keys ignored in scope 1:.:.:qfontdialog.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################
