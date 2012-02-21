TEMPLATE = subdirs

linux-g++-maemo: SUBDIRS += meego

contains(QT_CONFIG, evdev) {
    contains(QT_CONFIG, libudev) {
        SUBDIRS += evdevmouse evdevtouch evdevkeyboard
    }
}
