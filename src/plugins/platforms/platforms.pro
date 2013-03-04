TEMPLATE = subdirs

android:!android-no-sdk: SUBDIRS += android

SUBDIRS += minimal offscreen

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    ios: SUBDIRS += ios
    else: SUBDIRS += cocoa
}

win32: SUBDIRS += windows

qnx {
    SUBDIRS += qnx
}

contains(QT_CONFIG, eglfs) {
    SUBDIRS += eglfs
    SUBDIRS += minimalegl
}

contains(QT_CONFIG, directfb) {
    SUBDIRS += directfb
}

contains(QT_CONFIG, kms) {
    SUBDIRS += kms
}

contains(QT_CONFIG, linuxfb): SUBDIRS += linuxfb
