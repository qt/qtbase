# Generated from openglwidgets.pro.

#####################################################################
## OpenGLWidgets Module:
#####################################################################

qt_internal_add_module(OpenGLWidgets
    SOURCES
        qopenglwidget.cpp qopenglwidget.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::OpenGLPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::OpenGL
        Qt::Widgets
    GENERATE_CPP_EXPORTS
)
