TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

android: SUBDIRS += android

!android: SUBDIRS += minimal

!android:qtConfig(freetype): SUBDIRS += offscreen

qtConfig(xcb) {
    SUBDIRS += xcb
}

uikit:!watchos: SUBDIRS += ios
osx: SUBDIRS += cocoa

win32:!winrt: SUBDIRS += windows
winrt: SUBDIRS += winrt

qtConfig(direct2d) {
    SUBDIRS += direct2d
}

qnx {
    SUBDIRS += qnx
}

qtConfig(eglfs) {
    SUBDIRS += eglfs
    SUBDIRS += minimalegl
}

qtConfig(directfb) {
    SUBDIRS += directfb
}

qtConfig(linuxfb): SUBDIRS += linuxfb

unix:!android:!darwin: SUBDIRS += vnc

freebsd {
    SUBDIRS += bsdfb
}

haiku {
    SUBDIRS += haiku
}

qtConfig(mirclient): SUBDIRS += mirclient

qtConfig(integrityfb): SUBDIRS += integrity
