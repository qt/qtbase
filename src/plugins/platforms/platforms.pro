TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

android:!android-embedded: SUBDIRS += android

!wasm:!android: SUBDIRS += minimal

!wasm:!android:qtConfig(freetype): SUBDIRS += offscreen

qtConfig(xcb) {
    SUBDIRS += xcb
}

uikit:!watchos: SUBDIRS += ios
osx: SUBDIRS += cocoa

win32: SUBDIRS += windows

qtConfig(direct2d1_1):qtConfig(directwrite1) {
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

qtHaveModule(network):qtConfig(vnc): SUBDIRS += vnc

freebsd {
    SUBDIRS += bsdfb
}

haiku {
    SUBDIRS += haiku
}

wasm: SUBDIRS += wasm

qtConfig(integrityfb): SUBDIRS += integrity
