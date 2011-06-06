TEMPLATE = subdirs

#out of source tree plugin setup start
SUBDIRS += install_rules
SRCDIRS = \
    dnd \
    eglconvenience \
    fb_base \
    fontdatabases \
    glxconvenience \
    printersupport \
    wayland

srcdirs.files = $$SRCDIRS
srcdirs.path = $$[QT_INSTALL_DATA]/platforms

INSTALLS = srcdirs
#out of source tree plugin setup end

SUBDIRS += minimal

contains(QT_CONFIG, wayland) {
    SUBDIRS += wayland
}

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    SUBDIRS += cocoa
}
