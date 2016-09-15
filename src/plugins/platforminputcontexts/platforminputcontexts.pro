TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus
}

qtConfig(xcb): SUBDIRS += compose


