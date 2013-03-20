TEMPLATE = subdirs
qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus maliit
}
