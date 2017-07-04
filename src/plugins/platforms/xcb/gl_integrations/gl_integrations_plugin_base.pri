QT += core-private gui-private xcb_qpa_lib-private

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../

load(qt_build_paths)

# build with session management support
qtConfig(xcb-sm) {
    DEFINES += XCB_USE_SM
}

!qtConfig(system-xcb) {
    DEFINES += XCB_USE_RENDER
    QMAKE_USE += xcb-static xcb
} else {
    qtConfig(xkb): QMAKE_USE += xcb_xkb
    # to support custom cursors with depth > 1
    qtConfig(xcb-render) {
        DEFINES += XCB_USE_RENDER
    }
    QMAKE_USE += xcb_syslibs
}
