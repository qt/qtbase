# Generated from foreignwindows.pro.

#####################################################################
## foreignwindows Binary:
#####################################################################

add_qt_manual_test(foreignwindows
    SOURCES
        ../diaglib/eventfilter.cpp ../diaglib/eventfilter.h
        ../diaglib/nativewindowdump.h
        ../diaglib/qwindowdump.cpp ../diaglib/qwindowdump.h
        ../diaglib/textdump.cpp ../diaglib/textdump.h
        main.cpp
    DEFINES
        QT_DIAG_LIB
    INCLUDE_DIRECTORIES
        ../diaglib
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:foreignwindows.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(foreignwindows CONDITION WIN32
    SOURCES
        ../diaglib/nativewindowdump_win.cpp
    PUBLIC_LIBRARIES
        user32
)

extend_target(foreignwindows CONDITION UNIX
    SOURCES
        ../diaglib/nativewindowdump.cpp
)

extend_target(foreignwindows CONDITION QT_FEATURE_widgets
    SOURCES
        ../diaglib/debugproxystyle.cpp ../diaglib/debugproxystyle.h
        ../diaglib/logwidget.cpp ../diaglib/logwidget.h
        ../diaglib/qwidgetdump.cpp ../diaglib/qwidgetdump.h
    PUBLIC_LIBRARIES
        Qt::WidgetsPrivate
)

extend_target(foreignwindows CONDITION QT_FEATURE_opengl
    SOURCES
        ../diaglib/glinfo.cpp ../diaglib/glinfo.h
    PUBLIC_LIBRARIES
        Qt::OpenGL
        Qt::OpenGLWidgets
)
