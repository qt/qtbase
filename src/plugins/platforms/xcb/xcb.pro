TARGET = xcb

load(qpa/plugin)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT += core-private gui-private

SOURCES = \
        qxcbclipboard.cpp \
        qxcbconnection.cpp \
        qxcbintegration.cpp \
        qxcbkeyboard.cpp \
        qxcbmime.cpp \
        qxcbdrag.cpp \
        qxcbscreen.cpp \
        qxcbwindow.cpp \
        qxcbwindowsurface.cpp \
        qxcbwmsupport.cpp \
        main.cpp \
        qxcbnativeinterface.cpp

HEADERS = \
        qxcbclipboard.h \
        qxcbconnection.h \
        qxcbintegration.h \
        qxcbkeyboard.h \
        qxcbdrag.h \
        qxcbmime.h \
        qxcbobject.h \
        qxcbscreen.h \
        qxcbwindow.h \
        qxcbwindowsurface.h \
        qxcbwmsupport.h \
        qxcbnativeinterface.h

QT += gui-private core-private

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
            load(qpa/egl/convenience)
            load(qpa/egl/context)
            load(qpa/egl/xlibintegration)

            LIBS += -lEGL
        } else {
            DEFINES += XCB_USE_GLX
            load(qpa/glx/convenience)
            HEADERS += qglxintegration.h
            SOURCES += qglxintegration.cpp
        }
    }
}

LIBS += -lxcb -lxcb-image -lxcb-keysyms -lxcb-icccm -lxcb-sync

DEFINES += $$QMAKE_DEFINES_XCB QT_NO_XCB_XKB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB

load(qpa/fontdatabases/genericunix)
load(qpa/printersupport/genericunix)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
