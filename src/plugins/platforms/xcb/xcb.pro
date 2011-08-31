TARGET = xcb

load(qt_plugin)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT += core-private gui-private platformsupport-private

SOURCES = \
        qxcbclipboard.cpp \
        qxcbconnection.cpp \
        qxcbintegration.cpp \
        qxcbkeyboard.cpp \
        qxcbmime.cpp \
        qxcbdrag.cpp \
        qxcbscreen.cpp \
        qxcbwindow.cpp \
        qxcbbackingstore.cpp \
        qxcbwmsupport.cpp \
        main.cpp \
        qxcbnativeinterface.cpp \
        qxcbcursor.cpp \
        qxcbimage.cpp

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
        qxcbbackingstore.h \
        qxcbwmsupport.h \
        qxcbnativeinterface.h \
        qxcbcursor.h \
        qxcbimage.h

contains(QT_CONFIG, xcb-poll-for-queued-event) {
    DEFINES += XCB_POLL_FOR_QUEUED_EVENT
}

# needed by GLX, Xcursor, XLookupString, ...
contains(QT_CONFIG, xcb-xlib) {
    DEFINES += XCB_USE_XLIB
    LIBS += -lX11 -lX11-xcb
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
    LIBS += -lxcb-render -lxcb-render-util
}

#    DEFINES += XCB_USE_DRI2
contains(DEFINES, XCB_USE_DRI2) {
    LIBS += -lxcb-dri2 -lEGL

    CONFIG += link_pkgconfig
    PKGCONFIG += libdrm

    HEADERS += qdri2context.h
    SOURCES += qdri2context.cpp

} else {
    contains(QT_CONFIG, opengles2) {
        DEFINES += XCB_USE_EGL
        LIBS += -lEGL
        HEADERS += qxcbeglsurface.h
    } else:contains(QT_CONFIG, xcb-xlib) {
        DEFINES += XCB_USE_GLX
        HEADERS += qglxintegration.h
        SOURCES += qglxintegration.cpp
    }
}

LIBS += -lxcb -lxcb-image -lxcb-keysyms -lxcb-icccm -lxcb-sync -lxcb-xfixes

DEFINES += $$QMAKE_DEFINES_XCB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, dbus) {
DEFINES += XCB_USE_IBUS
QT += dbus
LIBS += -ldbus-1
}

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
