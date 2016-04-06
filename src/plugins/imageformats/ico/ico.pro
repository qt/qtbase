TARGET  = qico

HEADERS += main.h qicohandler.h
SOURCES += main.cpp qicohandler.cpp
OTHER_FILES += ico.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QICOPlugin
load(qt_plugin)
