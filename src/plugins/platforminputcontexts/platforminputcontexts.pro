TEMPLATE = subdirs

qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus maliit
}

unix:!macx:contains(QT_CONFIG, xkbcommon): {
    SUBDIRS += compose
}

