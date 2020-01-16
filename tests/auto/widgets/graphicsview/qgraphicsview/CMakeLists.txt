# Generated from qgraphicsview.pro.

#####################################################################
## tst_qgraphicsview Test:
#####################################################################

add_qt_test(tst_qgraphicsview
    SOURCES
        tst_qgraphicsview.cpp tst_qgraphicsview.h
        tst_qgraphicsview_2.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qgraphicsview.pro:<TRUE>:
# testcase.timeout = "500"

## Scopes:
#####################################################################

extend_target(tst_qgraphicsview CONDITION QT_FEATURE_opengl
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
