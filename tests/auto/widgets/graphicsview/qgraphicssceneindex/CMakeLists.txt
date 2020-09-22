# Generated from qgraphicssceneindex.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qgraphicssceneindex Test:
#####################################################################

qt_internal_add_test(tst_qgraphicssceneindex
    SOURCES
        tst_qgraphicssceneindex.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:qgraphicssceneindex.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
