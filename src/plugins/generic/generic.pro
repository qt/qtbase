TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

load(qfeatures)

qtConfig(evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}

qtConfig(tslib) {
    SUBDIRS += tslib
}

!contains(QT_DISABLED_FEATURES, udpsocket) {
    SUBDIRS += tuiotouch
}

qtConfig(libinput) {
    SUBDIRS += libinput
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
