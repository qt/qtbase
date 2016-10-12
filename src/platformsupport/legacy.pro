TARGET     = QtPlatformSupport
MODULE     = platformsupport

QT_FOR_CONFIG += gui-private
CONFIG += static internal_module

SOURCES += legacy.cpp

mods = \
    accessibility \
    cgl \
    clipboard \
    devicediscovery \
    egl \
    eventdispatcher \
    fb \
    fontdatabase \
    glx \
    graphics \
    input \
    linuxaccessibility \
    platformcompositor \
    service \
    theme

for (mod, mods) {
    mod = $${mod}_support-private
    qtHaveModule($$mod): \
        QT += $$mod
}

load(qt_module)
