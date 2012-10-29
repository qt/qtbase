TARGET = xcb

PLUGIN_TYPE = platforms
load(qt_plugin)

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
        qxcbimage.cpp \
        qxlibconvenience.cpp

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
        qxcbimage.h \
        qxlibconvenience.h

LIBS += -ldl

# needed by GLX, Xcursor, XLookupString, ...
contains(QT_CONFIG, xcb-xlib) {
    DEFINES += XCB_USE_XLIB
    LIBS += -lX11 -lX11-xcb

    *-maemo* {
        contains(QT_CONFIG, xinput2) {
            # XInput2 support for Harmattan.
            DEFINES += XCB_USE_XINPUT2_MAEMO
            SOURCES += qxcbconnection_maemo.cpp
            LIBS += -lXi
        }
        DEFINES += XCB_USE_MAEMO_WINDOW_PROPERTIES
    } else {
        contains(QT_CONFIG, xinput2) {
            DEFINES += XCB_USE_XINPUT2
            SOURCES += qxcbconnection_xi2.cpp
            LIBS += -lXi
        }
    }
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
    LIBS += -lxcb-render -lxcb-render-util -lXrender
}

contains(QT_CONFIG, opengl) {
    contains(QT_CONFIG, opengles2) {
        DEFINES += XCB_USE_EGL
        LIBS += -lEGL
        HEADERS += qxcbeglsurface.h

        # EGL on MeeGo 1.2 Harmattan needs this macro to map EGLNativeDisplayType
        # and other types to the correct X11 types
        DEFINES += SUPPORT_X11
    } else:contains(QT_CONFIG, xcb-xlib) {
        DEFINES += XCB_USE_GLX
        HEADERS += qglxintegration.h
        SOURCES += qglxintegration.cpp
        LIBS += $$QMAKE_LIBS_DYNLOAD
        contains(QT_CONFIG, xcb-glx) {
            DEFINES += XCB_HAS_XCB_GLX
            LIBS += -lxcb-glx
        }
    }
}

DEFINES += $$QMAKE_DEFINES_XCB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, dbus) {
DEFINES += XCB_USE_IBUS
QT += dbus
LIBS += -ldbus-1
}

OTHER_FILES += xcb.json README

contains(QT_CONFIG, xcb-qt) {
    DEFINES += XCB_USE_RENDER
    XCB_DIR = ../../../3rdparty/xcb
    INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/sysinclude
    LIBS += -lxcb -L ./xcb-static -l xcb-static
} else {
    LIBS += -lxcb -lxcb-image -lxcb-keysyms -lxcb-icccm -lxcb-sync -lxcb-xfixes -lxcb-shm -lxcb-randr
    !contains(DEFINES, QT_NO_SHAPE):LIBS += -lxcb-shape
}

