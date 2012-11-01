TARGET     = QtPlatformSupport
QT         = core-private gui-private

CONFIG += static internal_module
mac:LIBS += -lz

load(qt_module)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

include(cglconvenience/cglconvenience.pri)
include(dnd/dnd.pri)
include(eglconvenience/eglconvenience.pri)
include(eventdispatchers/eventdispatchers.pri)
include(fbconvenience/fbconvenience.pri)
include(fontdatabases/fontdatabases.pri)
include(glxconvenience/glxconvenience.pri)
include(input/input.pri)
include(devicediscovery/devicediscovery.pri)
include(services/services.pri)
include(themes/themes.pri)
include(linuxaccessibility/linuxaccessibility.pri)
