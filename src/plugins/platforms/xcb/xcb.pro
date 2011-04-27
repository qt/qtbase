TARGET = xcb

include(../../qpluginbase.pri)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

SOURCES = \
        qxcbconnection.cpp \
        qxcbintegration.cpp \
        qxcbkeyboard.cpp \
        qxcbscreen.cpp \
        qxcbwindow.cpp \
        qxcbwindowsurface.cpp \
        main.cpp \
        qxcbnativeinterface.cpp

HEADERS = \
        qxcbconnection.h \
        qxcbintegration.h \
        qxcbkeyboard.h \
        qxcbobject.h \
        qxcbscreen.h \
        qxcbwindow.h \
        qxcbwindowsurface.h \
        qxcbnativeinterface.h

contains(QT_CONFIG, opengl) {
    QT += opengl

#    DEFINES += XCB_USE_DRI2
    contains(DEFINES, XCB_USE_DRI2) {
        LIBS += -lxcb-dri2 -lxcb-xfixes -lEGL

        CONFIG += link_pkgconfig
        PKGCONFIG += libdrm

        HEADERS += qdri2context.h
        SOURCES += qdri2context.cpp

    } else {
        DEFINES += XCB_USE_XLIB
        LIBS += -lX11 -lX11-xcb

        contains(QT_CONFIG, opengles2) {
            DEFINES += XCB_USE_EGL
            HEADERS += \
                ../eglconvenience/qeglplatformcontext.h \
                ../eglconvenience/qeglconvenience.h \
                ../eglconvenience/qxlibeglintegration.h

            SOURCES += \
                ../eglconvenience/qeglplatformcontext.cpp \
                ../eglconvenience/qeglconvenience.cpp \
                ../eglconvenience/qxlibeglintegration.cpp

            LIBS += -lEGL
        } else {
            DEFINES += XCB_USE_GLX
            include (../glxconvenience/glxconvenience.pri)
            HEADERS += qglxintegration.h
            SOURCES += qglxintegration.cpp
        }
    }
}

LIBS += -lxcb -lxcb-image -lxcb-keysyms -lxcb-icccm -lxcb-sync

include (../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
