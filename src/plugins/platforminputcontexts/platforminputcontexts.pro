TEMPLATE = subdirs

qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus maliit
}

unix:!macx:!contains(DEFINES, QT_NO_XKBCOMMON): {
    SUBDIRS += compose
}

