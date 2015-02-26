TEMPLATE = subdirs

qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus
}

contains(QT_CONFIG, xcb-plugin): SUBDIRS += compose

contains(DEFINES,Q_OS_LINUX_TIZEN_MOBILE): SUBDIRS += tizenscim
