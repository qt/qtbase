TARGET = qxlib

load(qt_plugin)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT += core-private gui-private platformsupport-private

SOURCES = \
        main.cpp \
        qxlibintegration.cpp \
        qxlibbackingstore.cpp \
        qxlibwindow.cpp \
        qxlibcursor.cpp \
        qxlibscreen.cpp \
        qxlibkeyboard.cpp \
        qxlibclipboard.cpp \
        qxlibmime.cpp \
        qxlibstatic.cpp \
        qxlibdisplay.cpp \
        qxlibnativeinterface.cpp

HEADERS = \
        qxlibintegration.h \
        qxlibbackingstore.h \
        qxlibwindow.h \
        qxlibcursor.h \
        qxlibscreen.h \
        qxlibkeyboard.h \
        qxlibclipboard.h \
        qxlibmime.h \
        qxlibstatic.h \
        qxlibdisplay.h \
        qxlibnativeinterface.h

LIBS += -lX11 -lXext

mac {
    LIBS += -L/usr/X11/lib -lz -framework Carbon
}

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, opengl) {
    QT += opengl
    !contains(QT_CONFIG, opengles2) {
#        load(qpa/glx/convenience)
        HEADERS += qglxintegration.h
        SOURCES += qglxintegration.cpp
    } else { # There is no easy way to detect if we'r suppose to use glx or not
#        load(qpa/egl/context)
#        load(qpa/egl/convenience)
#        load(qpa/egl/xlibintegration)
        LIBS += -lEGL
    }
}

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
