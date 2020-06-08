# Generated from eglconvenience.pro.

#####################################################################
## EglSupport Module:
#####################################################################

qt_find_package(EGL) # special case

qt_add_module(EglSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qeglconvenience.cpp qeglconvenience_p.h
        qeglstreamconvenience.cpp qeglstreamconvenience_p.h
        qt_egl_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        EGL::EGL # special case
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:eglconvenience.pro:<TRUE>:
# MODULE = "egl_support"

## Scopes:
#####################################################################

qt_extend_target(EglSupport CONDITION QT_FEATURE_opengl
    SOURCES
        qeglpbuffer.cpp qeglpbuffer_p.h
        qeglplatformcontext.cpp qeglplatformcontext_p.h
)

qt_extend_target(EglSupport CONDITION QT_FEATURE_egl_x11
    SOURCES
        qxlibeglintegration.cpp qxlibeglintegration_p.h
    LIBRARIES
        X11::X11
)

qt_extend_target(EglSupport CONDITION NOT QT_FEATURE_egl_x11
    DEFINES
        QT_EGL_NO_X11
)

qt_extend_target(EglSupport CONDITION QT_FEATURE_dlopen
    LIBRARIES
        ${CMAKE_DL_LIBS}
)
