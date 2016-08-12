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
    QMAKE_USE += xcb_xlib

    contains(QT_CONFIG, xinput2) {
        DEFINES += XCB_USE_XINPUT2
        SOURCES += qxcbconnection_xi2.cpp
        QMAKE_USE += xinput2
        !isEmpty(QMAKE_XINPUT2_VERSION_MAJOR) {
            DEFINES += LIBXI_MAJOR=$$QMAKE_XINPUT2_VERSION_MAJOR \
                       LIBXI_MINOR=$$QMAKE_XINPUT2_VERSION_MINOR \
                       LIBXI_PATCH=$$QMAKE_XINPUT2_VERSION_PATCH
        }
    }
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
    QMAKE_USE += xcb_render
}

# build with session management support
contains(QT_CONFIG, xcb-sm) {
    DEFINES += XCB_USE_SM
    QMAKE_USE += x11sm
    SOURCES += qxcbsessionmanager.cpp
    HEADERS += qxcbsessionmanager.h
}

include(gl_integrations/gl_integrations.pri)

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, dbus-linked) {
    QT += dbus
}

contains(QT_CONFIG, xcb-qt) {
    DEFINES += XCB_USE_RENDER
    XCB_DIR = ../../../3rdparty/xcb
    INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/sysinclude
    LIBS += -L$$MODULE_BASE_OUTDIR/lib -lxcb-static$$qtPlatformTargetSuffix()
    QMAKE_USE += xcb
} else {
    LIBS += -lxcb-xinerama  ### there is no configure test for this!
    !contains(DEFINES, QT_NO_XKB): QMAKE_USE += xcb_xkb
    QMAKE_USE += xcb_syslibs
}

# libxkbcommon
contains(QT_CONFIG, xkbcommon-qt) {
    QT_CONFIG += use-xkbcommon-x11support
    include(../../../3rdparty/xkbcommon.pri)
} else {
    QMAKE_USE += xkbcommon
}

load(qt_module)
