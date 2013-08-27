TEMPLATE = subdirs

qtHaveModule(dbus) {
!mac:!win32:SUBDIRS += ibus
}

unix:!macx:!contains(DEFINES, QT_NO_XKBCOMMON): {
    SUBDIRS += compose
}

