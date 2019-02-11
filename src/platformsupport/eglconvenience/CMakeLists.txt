# Generated from eglconvenience.pro.

find_package(OpenGL)

#####################################################################
## EglSupport Module:
#####################################################################

add_qt_module(EglSupport
    STATIC
    SOURCES
        qeglconvenience.cpp qeglconvenience_p.h
        qeglstreamconvenience.cpp qeglstreamconvenience_p.h
        qt_egl_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_EGL_NO_X11
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        OpenGL::EGL
        ${CMAKE_DL_LIBS}
)

## Scopes:
#####################################################################

extend_target(EglSupport CONDITION QT_FEATURE_opengl
    SOURCES
        qeglpbuffer.cpp qeglpbuffer_p.h
        qeglplatformcontext.cpp qeglplatformcontext_p.h
)

extend_target(EglSupport CONDITION QT_FEATURE_xlib
    SOURCES
        qxlibeglintegration.cpp qxlibeglintegration_p.h
)
