TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

SUBDIRS = \
    edid \
    devicediscovery \
    fbconvenience

qtConfig(evdev)|qtConfig(tslib)|qtConfig(libinput)|qtConfig(integrityhid)|qtConfig(xkbcommon) {
    SUBDIRS += input
    input.depends += devicediscovery
}

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

