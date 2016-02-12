TARGET  = qgif

include(../../../gui/image/qgifhandler.pri)
INCLUDEPATH += ../../../gui/image
SOURCES += $$PWD/main.cpp
HEADERS += $$PWD/main.h
OTHER_FILES += gif.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QGifPlugin
load(qt_plugin)
