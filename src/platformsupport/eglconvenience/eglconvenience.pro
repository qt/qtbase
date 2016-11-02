TARGET = QtEglSupport
MODULE = egl_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

HEADERS += \
    qeglconvenience_p.h \
    qeglstreamconvenience_p.h \
    qt_egl_p.h

SOURCES += \
    qeglconvenience.cpp \
    qeglstreamconvenience.cpp

qtConfig(opengl) {
    HEADERS += \
        qeglplatformcontext_p.h \
        qeglpbuffer_p.h

    SOURCES += \
        qeglplatformcontext.cpp \
        qeglpbuffer.cpp
}

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

qtConfig(xlib) {
    HEADERS += \
        qxlibeglintegration_p.h
    SOURCES += \
        qxlibeglintegration.cpp
    LIBS_PRIVATE += $$QMAKE_LIBS_X11
}
CONFIG += egl

LIBS_PRIVATE += $$QMAKE_LIBS_DYNLOAD

load(qt_module)
