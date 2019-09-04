TARGET = mock5plugin

HEADERS += qmock5plugin.h
SOURCES += qmock5plugin.cpp
QT = mockplugins3

PLUGIN_TYPE = mockplugin
PLUGIN_CLASS_NAME = QMock5Plugin
PLUGIN_EXTENDS = -
load(qt_plugin)
