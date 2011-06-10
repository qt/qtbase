load(qt_module)
TARGET	   = QtPlatformSupport
QPRO_PWD   = $$PWD
QT         += core-private gui-private

CONFIG += module staticlib
MODULE_PRI = ../modules/qt_platformssupport.pri

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

include(../qbase.pri)

HEADERS += $$PWD/qtplatformsupportversion.h

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

include(dnd/dnd.pri)
include(eglconvenience/eglconvenience.pri)
include(fb_base/fb_base.pri)
include(fontdatabases/fontdatabases.pri)
include(glxconvenience/glxconvenience.pri)
include(printersupport/printersupport.pri)

