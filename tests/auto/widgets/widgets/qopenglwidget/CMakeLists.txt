# Generated from qopenglwidget.pro.

#####################################################################
## tst_qopenglwidget Test:
#####################################################################

add_qt_test(tst_qopenglwidget
    SOURCES
        tst_qopenglwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::OpenGLPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)
