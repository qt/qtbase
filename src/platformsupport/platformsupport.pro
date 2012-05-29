load(qt_build_config)
TARGET     = QtPlatformSupport
QT         = core-private gui-private

CONFIG += static
mac:LIBS += -lz -framework CoreFoundation -framework Carbon

load(qt_module_config)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

include(cglconvenience/cglconvenience.pri)
include(dnd/dnd.pri)
include(eglconvenience/eglconvenience.pri)
include(eventdispatchers/eventdispatchers.pri)
include(fb_base/fb_base.pri)
include(fontdatabases/fontdatabases.pri)
include(glxconvenience/glxconvenience.pri)
include(input/input.pri)
include(devicediscovery/devicediscovery.pri)
include(services/services.pri)
include(themes/themes.pri)
