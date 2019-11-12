qt_find_package(EGL) # special case

#####################################################################
## EglSupport Module:
#####################################################################

add_qt_module(EglSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qeglconvenience.cpp qeglconvenience_p.h
        qeglstreamconvenience.cpp qeglstreamconvenience_p.h
        qt_egl_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_EGL_NO_X11
    LIBRARIES
        ${CMAKE_DL_LIBS}
        Qt::CorePrivate
        Qt::GuiPrivate
        EGL::EGL # special case
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:eglconvenience.pro:<TRUE>:
# MODULE = "egl_support"

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
