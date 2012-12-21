TEMPLATE = subdirs
qtHaveModule(dbus) {
!macx:!win32:SUBDIRS += ibus maliit
}
