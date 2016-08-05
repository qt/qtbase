qtConfig(xlib) {
    qtConfig(opengl):!qtConfig(opengles2) {
        qtConfig(xrender): QMAKE_USE_PRIVATE += xrender
        LIBS_PRIVATE += $$QMAKE_LIBS_X11
        HEADERS += $$PWD/qglxconvenience_p.h
        SOURCES += $$PWD/qglxconvenience.cpp
    }
}
