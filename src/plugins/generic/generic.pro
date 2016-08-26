TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private network-private

qtConfig(evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}

qtConfig(tslib) {
    SUBDIRS += tslib
}

qtConfig(udpsocket) {
    SUBDIRS += tuiotouch
}

qtConfig(libinput) {
    SUBDIRS += libinput
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
