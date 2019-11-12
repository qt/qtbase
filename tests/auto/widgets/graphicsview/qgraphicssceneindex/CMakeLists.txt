# Generated from qgraphicssceneindex.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qgraphicssceneindex Test:
#####################################################################

add_qt_test(tst_qgraphicssceneindex
    SOURCES
        tst_qgraphicssceneindex.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qgraphicssceneindex.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
