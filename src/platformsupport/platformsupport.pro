TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

SUBDIRS = \
    eventdispatchers \
    devicediscovery \
    fbconvenience \
    themes

qtConfig(freetype)|darwin|win32: \
    SUBDIRS += fontdatabases

qtConfig(evdev)|qtConfig(tslib)|qtConfig(libinput)|qtConfig(integrityhid) {
    SUBDIRS += input
    input.depends += devicediscovery
}

unix:!darwin: \
    SUBDIRS += services

qtConfig(opengl): \
    SUBDIRS += platformcompositor
qtConfig(egl): \
    SUBDIRS += eglconvenience
qtConfig(xlib):qtConfig(opengl):!qtConfig(opengles2): \
    SUBDIRS += glxconvenience
qtConfig(kms): \
    SUBDIRS += kmsconvenience

qtConfig(accessibility) {
    SUBDIRS += accessibility
    qtConfig(accessibility-atspi-bridge) {
        SUBDIRS += linuxaccessibility
        linuxaccessibility.depends += accessibility
    }
}

darwin {
    SUBDIRS += \
        clipboard \
        graphics
    macos: \
        SUBDIRS += cglconvenience
}
