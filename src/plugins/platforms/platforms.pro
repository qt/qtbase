TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, wayland) {
    SUBDIRS += wayland
}

