TEMPLATE = subdirs

linux-g++-maemo: SUBDIRS += meego

contains(QT_CONFIG, evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}
