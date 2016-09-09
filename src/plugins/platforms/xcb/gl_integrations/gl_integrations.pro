TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(egl):qtConfig(egl_x11):qtConfig(opengl) {
    SUBDIRS += xcb_egl
}

qtConfig(xcb-xlib):qtConfig(opengl):!qtConfig(opengles2) {
    SUBDIRS += xcb_glx
}
