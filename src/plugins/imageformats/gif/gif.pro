TARGET  = qgif

SOURCES += main.cpp qgifhandler.cpp
HEADERS += main.h qgifhandler_p.h

OTHER_FILES += gif.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QGifPlugin
load(qt_plugin)
