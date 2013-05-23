contains(QT_CONFIG,xlib):contains(QT_CONFIG,xrender) {
    contains(QT_CONFIG,opengl):!contains(QT_CONFIG,opengles2) {
        LIBS_PRIVATE += $$QMAKE_LIBS_X11 -lXrender
        HEADERS += $$PWD/qglxconvenience_p.h
        SOURCES += $$PWD/qglxconvenience.cpp
    }
}
