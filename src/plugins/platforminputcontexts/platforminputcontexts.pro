TEMPLATE = subdirs

!contains(DEFINES,Q_OS_LINUX_TIZEN_MOBILE): {
    qtHaveModule(dbus) {
        !mac:!win32:SUBDIRS += ibus
    }

    contains(QT_CONFIG, xcb-plugin): SUBDIRS += compose
} else {
    SUBDIRS += tizenscim
}
