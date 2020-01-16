# Generated from opengl.pro.

#####################################################################
## OpenGL Module:
#####################################################################

qt_add_module(OpenGL
    SOURCES
        qopengldebug.cpp qopengldebug.h
        qopenglwindow.cpp qopenglwindow.h
        qtopenglglobal.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

## Scopes:
#####################################################################

qt_extend_target(OpenGL CONDITION QT_FEATURE_widgets
    SOURCES
        qopenglwidget.cpp qopenglwidget.h
    LIBRARIES
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_extend_target(OpenGL CONDITION NOT QT_FEATURE_opengles2
    SOURCES
        qopenglqueryhelper_p.h
        qopengltimerquery.cpp qopengltimerquery.h
)
qt_add_docs(OpenGL
    doc/qtopengl.qdocconf
)

