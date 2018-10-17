TARGET = mock3plugin

HEADERS += qmock3plugin.h
SOURCES += qmock3plugin.cpp
QT = mockplugins1

PLUGIN_TYPE = mockplugin
PLUGIN_CLASS_NAME = QMock3Plugin
PLUGIN_EXTENDS = mockplugins1 mockplugins2
load(qt_plugin)
