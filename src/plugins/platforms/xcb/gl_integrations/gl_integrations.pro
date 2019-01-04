TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(xcb-egl-plugin) {
    SUBDIRS += xcb_egl
}

qtConfig(xcb-glx-plugin) {
    SUBDIRS += xcb_glx
}
