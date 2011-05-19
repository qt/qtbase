TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, wayland) {
    SUBDIRS += wayland
}

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}
