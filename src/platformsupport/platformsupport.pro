TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

SUBDIRS = \
    edid \
    eventdispatchers \
    devicediscovery \
    fbconvenience

if(unix:!uikit:!macos)|qtConfig(xcb): \
    SUBDIRS += themes

if(qtConfig(freetype):!darwin)|win32: \
    SUBDIRS += fontdatabases

qtConfig(evdev)|qtConfig(tslib)|qtConfig(libinput)|qtConfig(integrityhid)|qtConfig(xkbcommon) {
    SUBDIRS += input
    input.depends += devicediscovery
}

if(unix:!uikit)|qtConfig(xcb): \
    SUBDIRS += services

qtConfig(egl): \
    SUBDIRS += eglconvenience
qtConfig(xlib):qtConfig(opengl):!qtConfig(opengles2): \
    SUBDIRS += glxconvenience
qtConfig(kms): \
    SUBDIRS += kmsconvenience

qtConfig(accessibility) {
    qtConfig(accessibility-atspi-bridge) {
        SUBDIRS += linuxaccessibility
    }
}

!android:linux*:qtHaveModule(dbus) \
    SUBDIRS += linuxofono

