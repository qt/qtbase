# Generated from qgraphicsview.pro.

#####################################################################
## tst_qgraphicsview Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsview
    SOURCES
        tst_qgraphicsview.cpp tst_qgraphicsview.h
        tst_qgraphicsview_2.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:qgraphicsview.pro:<TRUE>:
# testcase.timeout = "500"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qgraphicsview CONDITION QT_FEATURE_opengl
    PUBLIC_LIBRARIES
        Qt::OpenGL
        Qt::OpenGLWidgets
)
