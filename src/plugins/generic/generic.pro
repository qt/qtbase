TEMPLATE = subdirs

load(qfeatures)

contains(QT_CONFIG, evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}

contains(QT_CONFIG, tslib) {
    SUBDIRS += tslib
}

!contains(QT_DISABLED_FEATURES, udpsocket) {
    SUBDIRS += tuiotouch
}

contains(QT_CONFIG, libinput) {
    SUBDIRS += libinput
}
