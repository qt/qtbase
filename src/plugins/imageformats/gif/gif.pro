TARGET  = qgif

PLUGIN_TYPE = imageformats
load(qt_plugin)

include(../../../gui/image/qgifhandler.pri)
SOURCES += $$PWD/main.cpp
HEADERS += $$PWD/main.h
OTHER_FILES += gif.json
