QT += core-private gui-private xcb_qpa_lib-private

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../

load(qt_build_paths)

!qtConfig(system-xcb) {
    QMAKE_USE += xcb-static xcb
} else {
    qtConfig(xkb): QMAKE_USE += xcb_xkb
    qtConfig(xcb-render): QMAKE_USE += xcb_render
    QMAKE_USE += xcb_syslibs
}
