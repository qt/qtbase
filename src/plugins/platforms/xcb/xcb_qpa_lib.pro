TARGET     = QtXcbQpa
CONFIG += no_module_headers internal_module

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
        qxcbnativeinterface.cpp \
        qxcbcursor.cpp \
        qxcbimage.cpp \
        qxcbxsettings.cpp \
        qxcbsystemtraytracker.cpp

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
        qxcbxsettings.h \
        qxcbsystemtraytracker.h

DEFINES += QT_BUILD_XCB_PLUGIN
# needed by Xcursor ...
contains(QT_CONFIG, xcb-xlib) {
    DEFINES += XCB_USE_XLIB
    LIBS += -lX11 -lX11-xcb

    contains(QT_CONFIG, xinput2) {
        DEFINES += XCB_USE_XINPUT2
        SOURCES += qxcbconnection_xi2.cpp
        LIBS += -lXi
        !isEmpty(QMAKE_LIBXI_VERSION_MAJOR) {
            DEFINES += LIBXI_MAJOR=$$QMAKE_LIBXI_VERSION_MAJOR \
                       LIBXI_MINOR=$$QMAKE_LIBXI_VERSION_MINOR \
                       LIBXI_PATCH=$$QMAKE_LIBXI_VERSION_PATCH
        }
    }
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
    LIBS += -lxcb-render -lxcb-render-util
}

# build with session management support
contains(QT_CONFIG, xcb-sm) {
    DEFINES += XCB_USE_SM
    LIBS += -lSM -lICE
    SOURCES += qxcbsessionmanager.cpp
    HEADERS += qxcbsessionmanager.h
}

include(gl_integrations/gl_integrations.pri)

DEFINES += $$QMAKE_DEFINES_XCB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB
QMAKE_CFLAGS += $$QMAKE_CFLAGS_XCB

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, dbus-linked) {
    QT += dbus
    LIBS += $$QT_LIBS_DBUS
}

contains(QT_CONFIG, xcb-qt) {
    DEFINES += XCB_USE_RENDER
    XCB_DIR = ../../../3rdparty/xcb
    INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/sysinclude
    LIBS += -lxcb -L$$OUT_PWD/xcb-static -lxcb-static
} else {
    LIBS += -lxcb -lxcb-image -lxcb-icccm -lxcb-sync -lxcb-xfixes -lxcb-shm -lxcb-randr -lxcb-shape -lxcb-keysyms -lxcb-xinerama
    !contains(DEFINES, QT_NO_XKB):LIBS += -lxcb-xkb
}

# libxkbcommon
contains(QT_CONFIG, xkbcommon-qt) {
    QT_CONFIG += use-xkbcommon-x11support
    include(../../../3rdparty/xkbcommon.pri)
} else {
    LIBS += $$QMAKE_LIBS_XKBCOMMON
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XKBCOMMON
}

load(qt_module)
