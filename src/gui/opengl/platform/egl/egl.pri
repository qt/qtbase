HEADERS += \
    opengl/platform/egl/qeglconvenience_p.h \
    opengl/platform/egl/qeglstreamconvenience_p.h \
    opengl/platform/egl/qt_egl_p.h

SOURCES += \
    opengl/platform/egl/qeglconvenience.cpp \
    opengl/platform/egl/qeglstreamconvenience.cpp

qtConfig(opengl) {
    HEADERS += \
        opengl/platform/egl/qeglplatformcontext_p.h \
        opengl/platform/egl/qeglpbuffer_p.h

    SOURCES += \
        opengl/platform/egl/qeglplatformcontext.cpp \
        opengl/platform/egl/qeglpbuffer.cpp
}

qtConfig(egl_x11) {
    HEADERS += \
        opengl/platform/egl/qxlibeglintegration_p.h
    SOURCES += \
        opengl/platform/egl/qxlibeglintegration.cpp
    QMAKE_USE_PRIVATE += xlib
} else {
    # Avoid X11 header collision, use generic EGL native types
    DEFINES += QT_EGL_NO_X11
}
CONFIG += egl

qtConfig(dlopen): QMAKE_USE_PRIVATE += libdl
