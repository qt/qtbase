TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

SUBDIRS = \
    edid \
    eventdispatchers \
    devicediscovery \
    fbconvenience \
    themes

qtConfig(freetype)|darwin|win32: \
    SUBDIRS += fontdatabases

qtConfig(evdev)|qtConfig(tslib)|qtConfig(libinput)|qtConfig(integrityhid)|qtConfig(xkbcommon) {
    SUBDIRS += input
    input.depends += devicediscovery
}

if(unix:!uikit)|qtConfig(xcb): \
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
    win32:!winrt: SUBDIRS += windowsuiautomation
}

darwin {
    SUBDIRS += \
        clipboard \
        graphics
}

qtConfig(vulkan): \
    SUBDIRS += vkconvenience
