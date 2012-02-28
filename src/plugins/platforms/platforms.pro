TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac:contains(QT_CONFIG, coreservices): SUBDIRS += cocoa

win32: SUBDIRS += windows

qnx {
    SUBDIRS += qnx
}

contains(QT_CONFIG, eglfs) {
    SUBDIRS += eglfs
}

contains(QT_CONFIG, directfb) {
    SUBDIRS += directfb
}
