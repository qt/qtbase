TARGET = mock2plugin

HEADERS += qmock2plugin.h
SOURCES += qmock2plugin.cpp
QT = mockplugins1

PLUGIN_TYPE = mockplugin
PLUGIN_CLASS_NAME = QMock2Plugin
PLUGIN_EXTENDS = mockplugins1
load(qt_plugin)
