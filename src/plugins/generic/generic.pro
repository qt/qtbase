TEMPLATE = subdirs

*-maemo*: SUBDIRS += meego

contains(QT_CONFIG, evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}
