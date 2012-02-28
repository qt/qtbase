TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    SUBDIRS += cocoa
}

win32: SUBDIRS += windows

blackberry-armv7le-qcc {
    SUBDIRS += blackberry
}
