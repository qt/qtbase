TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private network-private

qtConfig(evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard
    qtConfig(tabletevent): \
        SUBDIRS += evdevtablet
}

qtConfig(tslib) {
    SUBDIRS += tslib
}

qtHaveModule(network):qtConfig(udpsocket) {
    SUBDIRS += tuiotouch
}

qtConfig(libinput) {
    SUBDIRS += libinput
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
