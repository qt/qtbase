qtConfig(evdev) {
    include($$PWD/evdevmouse/evdevmouse.pri)
    include($$PWD/evdevkeyboard/evdevkeyboard.pri)
    include($$PWD/evdevtouch/evdevtouch.pri)
    include($$PWD/evdevtablet/evdevtablet.pri)
}

qtConfig(tslib) {
    include($$PWD/tslib/tslib.pri)
}

qtConfig(libinput) {
    include($$PWD/libinput/libinput.pri)
}

qtConfig(evdev)|qtConfig(libinput) {
    include($$PWD/shared/shared.pri)
}
