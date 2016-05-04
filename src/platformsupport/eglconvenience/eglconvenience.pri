contains(QT_CONFIG,egl) {
    HEADERS += \
        $$PWD/qeglconvenience_p.h \
        $$PWD/qeglstreamconvenience_p.h

    SOURCES += \
        $$PWD/qeglconvenience.cpp \
        $$PWD/qeglstreamconvenience.cpp

    contains(QT_CONFIG,opengl) {
        HEADERS += $$PWD/qeglplatformcontext_p.h \
                   $$PWD/qeglpbuffer_p.h

        SOURCES += $$PWD/qeglplatformcontext.cpp \
                   $$PWD/qeglpbuffer.cpp
    }

    # Avoid X11 header collision
    DEFINES += MESA_EGL_NO_X11_HEADERS

    contains(QT_CONFIG,xlib) {
        HEADERS += \
            $$PWD/qxlibeglintegration_p.h
        SOURCES += \
            $$PWD/qxlibeglintegration.cpp
        LIBS_PRIVATE += $$QMAKE_LIBS_X11
    }
    CONFIG += egl
}
