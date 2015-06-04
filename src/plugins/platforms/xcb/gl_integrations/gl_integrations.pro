TEMPLATE = subdirs

contains(QT_CONFIG, egl): contains(QT_CONFIG, egl_x11): contains(QT_CONFIG, opengl) {
    SUBDIRS += xcb_egl
}

contains(QT_CONFIG, xcb-xlib): contains(QT_CONFIG, opengl): !contains(QT_CONFIG, opengles2) {
    SUBDIRS += xcb_glx
}
