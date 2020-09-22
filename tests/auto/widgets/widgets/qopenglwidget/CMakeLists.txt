# Generated from qopenglwidget.pro.

#####################################################################
## tst_qopenglwidget Test:
#####################################################################

qt_internal_add_test(tst_qopenglwidget
    LOWDPI # special case
    SOURCES
        tst_qopenglwidget.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLPrivate
        Qt::OpenGLWidgets
        Qt::Widgets
)
