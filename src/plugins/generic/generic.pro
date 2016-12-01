TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private network-private

qtConfig(thread): qtConfig(evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard
    qtConfig(tabletevent): \
        SUBDIRS += evdevtablet
}

qtConfig(tslib) {
    SUBDIRS += tslib
}

!emscripten:qtConfig(udpsocket) {
    SUBDIRS += tuiotouch
}

qtConfig(libinput) {
    SUBDIRS += libinput
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
