TARGET = mock4plugin

HEADERS += qmock4plugin.h
SOURCES += qmock4plugin.cpp
QT = mockplugins1

PLUGIN_TYPE = mockplugin
PLUGIN_CLASS_NAME = QMock4Plugin
PLUGIN_EXTENDS = -
load(qt_plugin)
