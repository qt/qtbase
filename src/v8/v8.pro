load(qt_module)

TARGET     = QtV8
QPRO_PWD   = $$PWD
QT         =

CONFIG += module
MODULE_PRI = ../modules/qt_v8.pri

win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000

load(qt_module_config)

# Remove includepaths that were added by qt_module_config.
# These cause compilation of V8 to fail because they appear before
# 3rdparty/v8/src; 3rdparty/v8/src/v8.h will then be "shadowed" by
# the public v8.h API header (they are not the same!).
INCLUDEPATH -= $$MODULE_PRIVATE_INCLUDES
INCLUDEPATH -= $$MODULE_PRIVATE_INCLUDES/$$TARGET
INCLUDEPATH -= $$MODULE_INCLUDES $$MODULE_INCLUDES/..

HEADERS += $$QT_SOURCE_TREE/src/v8/qtv8version.h

include(v8.pri)
