QT += core-private gui-private platformsupport-private xcb_qpa_lib-private

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../

# needed by Xcursor ...
contains(QT_CONFIG, xcb-xlib) {
    DEFINES += XCB_USE_XLIB
    contains(QT_CONFIG, xinput2) {
        DEFINES += XCB_USE_XINPUT2
    }
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
}

# build with session management support
contains(QT_CONFIG, xcb-sm) {
    DEFINES += XCB_USE_SM
}

DEFINES += $$QMAKE_DEFINES_XCB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, xcb-qt) {
    DEFINES += XCB_USE_RENDER
    XCB_DIR = $$clean_path($$PWD/../../../../3rdparty/xcb)
    INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/include/xcb $$XCB_DIR/sysinclude
    LIBS += -lxcb -L$$OUT_PWD/xcb-static -lxcb-static
} else {
    LIBS += -lxcb -lxcb-image -lxcb-icccm -lxcb-sync -lxcb-xfixes -lxcb-shm -lxcb-randr -lxcb-shape -lxcb-keysyms
    !contains(DEFINES, QT_NO_XKB):LIBS += -lxcb-xkb
}
