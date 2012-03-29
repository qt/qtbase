TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    SUBDIRS += cocoa
}

win32: SUBDIRS += windows

qnx {
    SUBDIRS += qnx
}

contains(QT_CONFIG, eglfs) {
    SUBDIRS += eglfs
}
