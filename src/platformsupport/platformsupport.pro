TARGET     = QtPlatformSupport
QT         = core-private gui-private

CONFIG += static internal_module
mac:LIBS_PRIVATE += -lz

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

include(cglconvenience/cglconvenience.pri)
include(eglconvenience/eglconvenience.pri)
include(eventdispatchers/eventdispatchers.pri)
include(fbconvenience/fbconvenience.pri)
include(fontdatabases/fontdatabases.pri)
include(glxconvenience/glxconvenience.pri)
include(input/input.pri)
include(devicediscovery/devicediscovery.pri)
include(services/services.pri)
include(themes/themes.pri)
include(accessibility/accessibility.pri)
include(linuxaccessibility/linuxaccessibility.pri)
include(clipboard/clipboard.pri)
include(platformcompositor/platformcompositor.pri)
contains(QT_CONFIG, dbus) {
    include(dbusmenu/dbusmenu.pri)
    include(dbustray/dbustray.pri)
}
ios: include(graphics/graphics.pri)

load(qt_module)
