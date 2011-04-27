TARGET = qxlib

include(../../qpluginbase.pri)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

SOURCES = \
        main.cpp \
        qxlibintegration.cpp \
        qxlibwindowsurface.cpp \
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
        qxlibwindowsurface.h \
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

include (../fontdatabases/genericunix/genericunix.pri)

contains(QT_CONFIG, opengl) {
    QT += opengl
    !contains(QT_CONFIG, opengles2) {
        include (../glxconvenience/glxconvenience.pri)
        HEADERS += qglxintegration.h
        SOURCES += qglxintegration.cpp
    } else { # There is no easy way to detect if we'r suppose to use glx or not
        HEADERS += \
            ../eglconvenience/qeglplatformcontext.h \
            ../eglconvenience/qeglconvenience.h \
            ../eglconvenience/qxlibeglintegration.h

        SOURCES += \
            ../eglconvenience/qeglplatformcontext.cpp \
            ../eglconvenience/qeglconvenience.cpp \
            ../eglconvenience/qxlibeglintegration.cpp
        LIBS += -lEGL
    }
}

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
