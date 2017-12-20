TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard
    qtConfig(tabletevent): \
        SUBDIRS += evdevtablet
}

qtConfig(tslib) {
    SUBDIRS += tslib
}

qtConfig(tuiotouch) {
    SUBDIRS += tuiotouch
}

qtConfig(libinput) {
    SUBDIRS += libinput
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
